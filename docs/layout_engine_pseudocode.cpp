// Pseudocódigo: Layout Engine - Como Funciona o Cálculo de Página

#include <vector>
#include <string>

// ============================================================================
// ESTRUTURAS DE DADOS
// ============================================================================

struct ContentNode {
  enum Type { TEXT, BLOCK, INLINE, IMAGE };
  Type type;
  std::string text_utf8;

  struct ComputedStyle {
    int margin_left, margin_right, margin_top, margin_bottom;
    int text_indent;
    int font_size;
    int line_height;
    bool preformatted;
  } style;

  std::vector<ContentNode*> children;
  ContentNode* parent;
};

struct LayoutMetrics {
  int screen_width;
  int screen_height;
  int base_margin_left;
  int base_margin_right;
  int base_margin_top;
  int base_margin_bottom;
  int line_spacing;
};

struct PageStart {
  ContentNode* node;
  size_t char_offset;  // onde retomar dentro do node
};

struct ShapedGlyph {
  uint32_t codepoint;
  int advance;
  size_t byte_offset;
};

// ============================================================================
// LAYOUT ENGINE
// ============================================================================

class LayoutEngine {
private:
  struct BlockContext {
    int effective_margin_left;
    int effective_margin_right;
    int text_indent;
    bool is_first_line;
  };

  struct LineFragment {
    std::vector<ShapedGlyph> glyphs;
    int x, y, width;
    ContentNode* source_node;
    size_t text_start, text_length;
  };

  struct LayoutContext {
    int pen_x, pen_y;
    int current_line_height;
    std::vector<BlockContext> block_stack;
    std::vector<LineFragment> current_line;
    bool line_has_content;
  };

public:
  struct LayoutPage {
    struct LayoutLine {
      std::vector<LineFragment> fragments;
      int y, height;
    };
    std::vector<LayoutLine> lines;

    // Onde esta página terminou
    PageStart end_position;
  };

  LayoutPage ComputePage(
    const ContentNode* start_node,
    size_t start_offset,
    const LayoutMetrics& metrics
  ) {
    LayoutPage page;
    LayoutContext ctx;

    // Inicializa pen na posição inicial da página
    ctx.pen_x = metrics.base_margin_left;
    ctx.pen_y = metrics.base_margin_top;
    ctx.line_has_content = false;

    // Retoma do ponto especificado
    bool started = false;
    LayoutNode(start_node, start_offset, &started, ctx, page, metrics);

    // Finaliza última linha se houver
    if (ctx.line_has_content) {
      CommitLine(ctx, page);
    }

    return page;
  }

private:
  void LayoutNode(
    const ContentNode* node,
    size_t resume_offset,
    bool* started,
    LayoutContext& ctx,
    LayoutPage& page,
    const LayoutMetrics& metrics
  ) {
    if (!node) return;

    // Se ainda não começamos a processar, pula até o nó correto
    if (!*started) {
      if (node != /* start_node */) {
        // Não é o nó de início, procura em children
        for (auto* child : node->children) {
          LayoutNode(child, resume_offset, started, ctx, page, metrics);
          if (*started) return;
        }
        return;
      }
      *started = true;
    }

    if (node->type == ContentNode::BLOCK) {
      LayoutBlockNode(node, resume_offset, ctx, page, metrics);
    } else if (node->type == ContentNode::TEXT) {
      LayoutTextNode(node, resume_offset, ctx, page, metrics);
    }

    // Processa children
    for (auto* child : node->children) {
      size_t child_offset = 0; // sempre começa do início para children
      LayoutNode(child, child_offset, started, ctx, page, metrics);

      // Verifica se página ficou cheia
      if (PageIsFull(ctx, metrics)) {
        return;
      }
    }
  }

  void LayoutBlockNode(
    const ContentNode* node,
    size_t resume_offset,
    LayoutContext& ctx,
    LayoutPage& page,
    const LayoutMetrics& metrics
  ) {
    // Aplica margin-top (se não for resumo)
    if (resume_offset == 0) {
      // Colapsa margens: max(margin_bottom anterior, margin_top atual)
      ctx.pen_y += node->style.margin_top;

      // Verifica se já passou do limite da página
      if (ctx.pen_y > metrics.screen_height - metrics.base_margin_bottom) {
        // Página cheia antes de começar bloco
        return;
      }
    }

    // Empurra contexto de bloco
    BlockContext block_ctx;
    block_ctx.effective_margin_left =
      metrics.base_margin_left + node->style.margin_left;
    block_ctx.effective_margin_right =
      metrics.base_margin_right + node->style.margin_right;
    block_ctx.text_indent = node->style.text_indent;
    block_ctx.is_first_line = (resume_offset == 0);

    ctx.block_stack.push_back(block_ctx);

    // Reseta pen.x para nova margem
    ctx.pen_x = block_ctx.effective_margin_left;
    if (block_ctx.is_first_line && block_ctx.text_indent > 0) {
      ctx.pen_x += block_ctx.text_indent;
    }
  }

  void LayoutTextNode(
    const ContentNode* node,
    size_t resume_offset,
    LayoutContext& ctx,
    LayoutPage& page,
    const LayoutMetrics& metrics
  ) {
    const std::string& text = node->text_utf8;

    // Shape do texto para obter glyphs
    std::vector<ShapedGlyph> glyphs;
    ShapeText(text.c_str() + resume_offset,
              text.size() - resume_offset,
              node->style.font_size,
              &glyphs);

    // Obtém contexto de bloco atual
    const BlockContext& block = ctx.block_stack.back();
    int max_width = metrics.screen_width -
                    block.effective_margin_left -
                    block.effective_margin_right;

    // Processa glyphs palavra por palavra
    size_t glyph_idx = 0;
    while (glyph_idx < glyphs.size()) {
      // Pula espaços iniciais de linha
      if (!ctx.line_has_content) {
        while (glyph_idx < glyphs.size() && IsSpace(glyphs[glyph_idx])) {
          glyph_idx++;
        }
      }

      // Encontra próxima palavra
      size_t word_start = glyph_idx;
      size_t word_end = word_start;
      int word_width = 0;

      while (word_end < glyphs.size() && !IsSpace(glyphs[word_end])) {
        word_width += glyphs[word_end].advance;
        word_end++;
      }

      if (word_end == word_start) break; // sem mais palavras

      // Adiciona espaço se não for primeira palavra da linha
      int space_width = 0;
      if (ctx.line_has_content) {
        space_width = GetSpaceWidth(node->style.font_size);
      }

      // Verifica se palavra cabe na linha
      if (ctx.line_has_content &&
          ctx.pen_x + space_width + word_width >
          block.effective_margin_left + max_width) {

        // Não cabe: finaliza linha atual
        CommitLine(ctx, page);

        // Verifica se nova linha cabe na página
        if (PageIsFull(ctx, metrics)) {
          // Salva posição de retomada
          page.end_position.node = const_cast<ContentNode*>(node);
          page.end_position.char_offset = glyphs[word_start].byte_offset;
          return;
        }

        // Reseta pen para nova linha
        ctx.pen_x = block.effective_margin_left;
        // Não usa text_indent em linhas subsequentes
        ctx.line_has_content = false;
        space_width = 0; // sem espaço no início de linha
      }

      // Adiciona palavra à linha
      LineFragment frag;
      frag.x = ctx.pen_x + space_width;
      frag.y = ctx.pen_y;
      frag.width = word_width;
      frag.source_node = const_cast<ContentNode*>(node);
      frag.text_start = glyphs[word_start].byte_offset;
      frag.text_length = (word_end < glyphs.size())
        ? (glyphs[word_end].byte_offset - glyphs[word_start].byte_offset)
        : (text.size() - glyphs[word_start].byte_offset);

      for (size_t i = word_start; i < word_end; i++) {
        frag.glyphs.push_back(glyphs[i]);
      }

      ctx.current_line.push_back(frag);
      ctx.pen_x += space_width + word_width;
      ctx.line_has_content = true;
      ctx.current_line_height = std::max(ctx.current_line_height,
                                         node->style.line_height);

      glyph_idx = word_end;

      // Pula espaços após palavra
      while (glyph_idx < glyphs.size() && IsSpace(glyphs[glyph_idx])) {
        glyph_idx++;
      }
    }
  }

  void CommitLine(LayoutContext& ctx, LayoutPage& page) {
    if (!ctx.line_has_content) return;

    LayoutPage::LayoutLine line;
    line.y = ctx.pen_y;
    line.height = ctx.current_line_height;
    line.fragments = ctx.current_line;

    page.lines.push_back(line);

    // Avança pen.y para próxima linha
    ctx.pen_y += ctx.current_line_height + /* line_spacing */ 3;

    // Reseta linha atual
    ctx.current_line.clear();
    ctx.line_has_content = false;
    ctx.current_line_height = 0;

    // Marca que primeira linha já passou
    if (!ctx.block_stack.empty()) {
      ctx.block_stack.back().is_first_line = false;
    }
  }

  bool PageIsFull(const LayoutContext& ctx, const LayoutMetrics& metrics) {
    int available_height = metrics.screen_height -
                          metrics.base_margin_bottom -
                          8; // pagination guard

    // Verifica se há espaço para mais uma linha
    return ctx.pen_y + ctx.current_line_height > available_height;
  }

  // Stubs para funções auxiliares
  void ShapeText(const char* text, size_t len, int font_size,
                 std::vector<ShapedGlyph>* out) {
    // Chama text_layout_utils::ShapeTextRunUtf8()
  }

  bool IsSpace(const ShapedGlyph& g) {
    return g.codepoint == ' ' || g.codepoint == '\t';
  }

  int GetSpaceWidth(int font_size) {
    // Retorna largura do espaço para fonte dada
    return font_size / 3;
  }
};

// ============================================================================
// EXEMPLO DE USO
// ============================================================================

void RenderBook() {
  // 1. Parse do EPUB gera DocumentTree
  ContentNode* root = ParseEPUB("book.epub");

  // 2. Configurações de layout
  LayoutMetrics metrics;
  metrics.screen_width = 400;
  metrics.screen_height = 240;
  metrics.base_margin_left = 16;
  metrics.base_margin_right = 16;
  metrics.base_margin_top = 12;
  metrics.base_margin_bottom = 36;

  // 3. Layout engine
  LayoutEngine engine;

  // 4. Calcula primeira página (começa do início)
  auto page0 = engine.ComputePage(root, 0, metrics);

  // 5. Renderiza página 0
  for (const auto& line : page0.lines) {
    for (const auto& frag : line.fragments) {
      // Desenha glyphs em (frag.x, frag.y)
      DrawTextFragment(frag);
    }
  }

  // 6. Usuário avança → calcula próxima página
  // Retoma exatamente de onde página 0 terminou
  auto page1 = engine.ComputePage(
    page0.end_position.node,
    page0.end_position.char_offset,
    metrics
  );

  // ✅ Contexto preservado! Margens, indentação, tudo correto
}

// ============================================================================
// COMPARAÇÃO: SISTEMA ATUAL vs NOVA ARQUITETURA
// ============================================================================

/*

SISTEMA ATUAL:
--------------
Parse → EmitFlowedShapedText() → Emite tokens para Page.buf[]
  - Toma decisões de paginação DURANTE parsing
  - Emite TEXT_LINE_START_X para margens customizadas
  - Problema: contexto perdido entre páginas

  <p style="margin-left: 40px">Texto longo...</p>

  Página 1: [emite TEXT_LINE_START_X, 56]  → x = 16 + 40
  Página 2: [perdeu contexto, usa x = 16] ❌ CORTADO!

  Solução atual: DESABILITAR TEXT_LINE_START_X
  Resultado: Indentação CSS não funciona ❌


NOVA ARQUITETURA:
-----------------
Parse → ContentNode tree (COM margin_left preservado)
Layout → Calcula página N com contexto completo
Render → Desenha usando posições absolutas

  ContentNode { style.margin_left = 40, text = "Texto longo..." }

  Página 1:
    block_stack = [{ effective_margin_left: 56 }]
    Fragmentos em x=56 ✅
    Salva end_position = {node, offset=100}

  Página 2:
    Retoma de end_position
    block_stack = [{ effective_margin_left: 56 }]  ← PRESERVADO!
    Fragmentos em x=56 ✅

  Resultado: Indentação funciona corretamente ✅

*/
