# Proposta: Nova Arquitetura de Reflow

## Problemas da Arquitetura Atual

1. **Layout prematuro**: Paginação acontece durante parsing, decisões "assadas" no buffer
2. **Perda de contexto**: Indentação CSS, margens de bloco perdidas entre páginas
3. **Inflexibilidade**: Não pode recalcular se usuário mudar fonte/margens
4. **Bugs de hifenização**: Caracteres desapareciam porque implementação incompleta

## Arquitetura Proposta: Layout Sob Demanda (Kindle-like)

### Princípios

- **Separation of Concerns**: Documento ≠ Layout ≠ Renderização
- **Lazy Evaluation**: Calcula página N apenas quando necessário
- **Stateful Context**: Mantém box model completo durante flow
- **Recalculable**: Pode invalidar e recalcular se settings mudarem

---

## CAMADA 1: Document Tree (Persistente)

### ContentNode (novo)

```cpp
struct ContentNode {
  enum Type {
    TEXT,
    BLOCK,      // <p>, <div>, <section>
    INLINE,     // <span>, <em>, <strong>
    IMAGE,
    LIST_ITEM,
    TABLE_CELL
  };

  Type type;

  // Texto UTF-8 original (para nós TEXT)
  std::string text_utf8;

  // Estilo computado (após cascata CSS)
  struct ComputedStyle {
    int font_size;
    int font_weight;      // 400=normal, 700=bold
    int font_style;       // 0=normal, 1=italic

    int margin_top;
    int margin_bottom;
    int margin_left;
    int margin_right;

    int padding_top;
    int padding_bottom;
    int padding_left;
    int padding_right;

    int text_indent;      // primeiro-linha
    int line_height;

    u16 text_color;
    u16 bg_color;

    u8 text_align;        // 0=left, 1=center, 2=right, 3=justify
    u8 text_transform;    // 0=none, 1=uppercase, 2=lowercase

    bool is_rtl;
    bool preformatted;
  } style;

  // Hierarquia
  std::vector<ContentNode*> children;
  ContentNode* parent;

  // Link (se aplicável)
  u16 href_id;
  bool is_link;
};

struct DocumentTree {
  ContentNode* root;
  std::vector<ContentNode*> all_nodes;  // ownership

  // Bookmarks/TOC apontam para nodes, não offsets
  struct Bookmark {
    ContentNode* node;
    size_t char_offset_in_node;
  };
};
```

### Vantagens

- **Preserva formatação original**: Margens, indentação, tudo está no estilo
- **Referencável**: Bookmarks apontam para nodes, não bytes
- **Recalculável**: Mudar fonte → reconstrói layout, documento intacto

---

## CAMADA 2: Layout Engine (Sob Demanda)

### LayoutContext (estado durante flow)

```cpp
struct LayoutContext {
  // Configurações de viewport
  int screen_width;
  int screen_height;
  int base_margin_left;
  int base_margin_right;
  int base_margin_top;
  int base_margin_bottom;

  // Estado atual do "pen" (cursor de layout)
  int pen_x;
  int pen_y;
  int current_line_height;
  int current_line_baseline;

  // Box model stack (para margens colapsáveis)
  struct BlockContext {
    int margin_left;
    int margin_right;
    int text_indent;
    bool first_line;
  };
  std::vector<BlockContext> block_stack;

  // Linha sendo construída
  struct LineFragment {
    ContentNode* source_node;
    size_t text_start;          // offset UTF-8 no node
    size_t text_length;
    std::vector<text_layout_utils::ShapedGlyph> glyphs;
    int x;
    int y;
    int width;
    int baseline;
    u16 color;
  };
  std::vector<LineFragment> current_line;

  // Função de medição
  text_layout_utils::MeasureCodepointFn measure_fn;
  void* measure_ctx;
};
```

### Algoritmo de Paginação

```cpp
struct LayoutPage {
  int page_number;

  // Linhas que cabem nesta página
  struct LayoutLine {
    std::vector<LayoutContext::LineFragment> fragments;
    int y;
    int height;
  };
  std::vector<LayoutLine> lines;

  // Posição no documento (para retomar)
  struct PageEnd {
    ContentNode* last_node;
    size_t char_offset;
  } end_position;

  // Links renderizados (para touch)
  std::vector<Page::InlineLinkRenderEntry> links;
};

class LayoutEngine {
public:
  // Calcula página N partindo de uma posição
  LayoutPage ComputePage(
    const DocumentTree& doc,
    const PageStart& start_pos,
    const LayoutMetrics& metrics
  );

private:
  // Processa um nó de conteúdo
  void LayoutNode(
    ContentNode* node,
    LayoutContext& ctx,
    LayoutPage& page
  );

  // Tenta adicionar texto à linha atual
  // Retorna true se coube, false se precisa quebrar
  bool TryAddToLine(
    const std::string& utf8,
    const ContentNode::ComputedStyle& style,
    LayoutContext& ctx
  );

  // Finaliza linha atual e avança pen.y
  void CommitLine(
    LayoutContext& ctx,
    LayoutPage& page
  );

  // Verifica se linha cabe na página
  bool LineWouldFit(
    int pen_y,
    int line_height,
    const LayoutMetrics& metrics
  );

  // Aplica margens de bloco (com collapse)
  void ApplyBlockMargins(
    const ContentNode::ComputedStyle& style,
    LayoutContext& ctx
  );
};
```

### Exemplo de Uso

```cpp
LayoutEngine engine;
LayoutMetrics metrics = GetCurrentMetrics();  // fonte, margens, telas

// Página 0: começa do início
PageStart start{doc.root, 0};
LayoutPage page0 = engine.ComputePage(doc, start, metrics);

// Renderiza página 0
RenderPage(page0);

// Usuário avança → página 1
// Retoma exatamente de onde página 0 terminou
PageStart start1{page0.end_position.last_node, page0.end_position.char_offset};
LayoutPage page1 = engine.ComputePage(doc, start1, metrics);
```

### Hifenização Correta

```cpp
struct HyphenationResult {
  size_t break_position;  // onde quebrar (caracteres UTF-8)
  bool needs_hyphen;      // se deve inserir '-'
};

// Dicionário + regras morfológicas
class HyphenationEngine {
public:
  // Tenta hifenizar palavra em UTF-8
  std::vector<HyphenationResult> FindHyphenationPoints(
    const std::string& word_utf8,
    const char* lang  // "pt", "en", etc.
  );
};

// No layout:
if (word_overflows) {
  auto hyph_points = hyphen_engine.FindHyphenationPoints(word, "pt");
  for (auto& hp : hyph_points) {
    if (FitsWithHyphen(hp)) {
      // Adiciona "palavra_parte-"
      AddToLine(word.substr(0, hp.break_position) + "-");
      // Próxima linha continua com "resto"
      pending_text = word.substr(hp.break_position);
      break;
    }
  }
}
```

---

## CAMADA 3: Render Cache

### Cache de Páginas Calculadas

```cpp
struct PageCache {
  struct CacheKey {
    int page_number;
    int font_size;
    int margin_left;
    int margin_right;
    // ... outras settings relevantes

    bool operator<(const CacheKey& other) const {
      // comparação para std::map
    }
  };

  std::map<CacheKey, LayoutPage> pages;

  // Invalida cache se settings mudaram
  void InvalidateIfChanged(const LayoutMetrics& new_metrics);

  // Busca página, calcula se não estiver em cache
  LayoutPage& GetPage(
    int page_number,
    const DocumentTree& doc,
    const LayoutMetrics& metrics,
    LayoutEngine& engine
  );
};
```

---

## Migração Gradual

### Fase 1: Construir Document Tree
- Modificar parser XML para construir ContentNodes em vez de emitir tokens
- Manter sistema antigo funcionando em paralelo

### Fase 2: Implementar Layout Engine
- Começar com layout simples (sem tabelas, sem RTL)
- Validar contra sistema antigo

### Fase 3: Cache e Otimização
- Adicionar cache de páginas
- Otimizar shaping (cache de glyphs por fonte/tamanho)

### Fase 4: Features Avançadas
- RTL com BiDi correto
- Tabelas com column balancing
- Imagens inline com text wrap
- Hifenização com dicionário

---

## Benefícios

1. **Sem perda de contexto**: Margens, indentação preservadas entre páginas
2. **Hifenização correta**: Pode tentar múltiplos pontos sem "perder" caracteres
3. **Recalculável**: Usuário muda fonte → invalida cache, recalcula
4. **Debugging**: Pode renderizar document tree visualmente
5. **Bookmarks estáveis**: Apontam para nodes, não offsets que mudam
6. **Progressive rendering**: Calcula página N sem precisar calcular 0..N-1

---

## Comparação com Sistema Atual

| Aspecto | Sistema Atual | Nova Arquitetura |
|---------|--------------|------------------|
| Paginação | Durante parsing (antecipada) | Sob demanda por página |
| Contexto CSS | Perdido entre páginas | Preservado no DocumentNode |
| Recálculo | Precisa re-parse completo | Invalida cache, recalcula layout |
| Hifenização | Removida (buggy) | Correta com dicionário |
| Indentação | Desabilitada (causava bugs) | Funciona corretamente |
| Bookmarks | Offset em buffer (instável) | Ponteiro para node (estável) |
| Memória | Buffer grande com tudo | Document tree + cache de ~5 páginas |

---

## Exemplo Prático

**Documento:**
```html
<p style="margin-left: 40px; text-indent: 20px;">
  Esta é uma parágrafo muito longo que certamente vai
  quebrar para múltiplas linhas e páginas.
</p>
```

**Sistema Atual:**
```
Página 1:
  [margem base + 40px] "Esta é uma parágrafo muito longo que"
  [emite TEXT_LINE_START_X com offset]

Página 2:
  [PROBLEMA: perdeu contexto de margin-left]
  [margem base apenas] "certamente vai quebrar para múltiplas"
  ❌ Texto cortado na margem errada!
```

**Nova Arquitetura:**
```
ContentNode {
  type: BLOCK,
  style: { margin_left: 40, text_indent: 20 },
  text: "Esta é uma parágrafo muito longo..."
}

LayoutPage 1:
  ctx.block_stack = [{ margin_left: 40, text_indent: 20, first_line: true }]
  Line 0: x=base+40+20, "Esta é uma parágrafo muito longo que"
  → página cheia, salva end_position = {node, offset=37}

LayoutPage 2:
  Retoma de {node, offset=37}
  ctx.block_stack = [{ margin_left: 40, text_indent: 0, first_line: false }]
  Line 0: x=base+40, "certamente vai quebrar para múltiplas"
  ✅ Margem correta preservada!
```

---

## Próximos Passos

1. Validar arquitetura com você
2. Implementar protótipo de ContentNode + parser
3. Layout engine básico (sem RTL, sem tabelas)
4. Benchmark: comparar performance com sistema atual
5. Migração gradual
