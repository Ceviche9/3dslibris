// FASE 1: Integração do DocumentTree com Parser XML
// Mostra como book_xml_parser.cpp seria modificado

#include "book/content_node.h"
#include "book/book_xml_parser_support.h"

// ============================================================================
// NOVO: Parser State com DocumentTree
// ============================================================================

struct ParserStateWithTree {
  // Compatibilidade com código existente
  parsedata_t* legacy_parse_data;  // mantém durante transição

  // NOVO: Árvore de documento
  content_tree::DocumentTree* doc_tree;

  // NOVO: Nó atual sendo construído
  content_tree::ContentNode* current_node;

  // NOVO: Stack de nós abertos (para elementos aninhados)
  std::vector<content_tree::ContentNode*> node_stack;

  // NOVO: Buffer temporário para coletar texto
  std::string text_buffer;

  // Estilo atual (computado via cascata CSS)
  content_tree::ComputedStyle current_style;

  // Stack de estilos (para herança)
  std::vector<content_tree::ComputedStyle> style_stack;
};

// ============================================================================
// EXEMPLO: Callback de abertura de tag
// ============================================================================

// Função existente: start() em book_xml_parser.cpp
// Seria modificada para também construir DocumentTree

void StartElementCallback(void *data, const char *name, const char **attr) {
  auto* state = static_cast<ParserStateWithTree*>(data);

  // === CÓDIGO LEGADO (mantém funcionando) ===
  if (state->legacy_parse_data) {
    // Chama lógica antiga...
    parse_start_element(state->legacy_parse_data, name, attr);
  }

  // === NOVO: Construção da DocumentTree ===
  if (state->doc_tree) {
    FlushTextBuffer(state);  // finaliza texto pendente

    // Determina tipo do elemento
    content_tree::ContentNode::Type node_type = DetermineNodeType(name);

    // Cria novo nó
    auto* node = state->doc_tree->CreateNode(node_type);
    node->tag_name = name;

    // Herda estilo do pai (cascata CSS)
    if (!state->style_stack.empty()) {
      node->style = state->style_stack.back();
    }

    // Aplica atributos e CSS inline
    ApplyAttributesAndStyle(node, name, attr, state);

    // Adiciona ao pai (se houver)
    if (!state->node_stack.empty()) {
      state->node_stack.back()->AddChild(node);
    } else {
      // Primeiro elemento → adiciona à raiz
      state->doc_tree->root->AddChild(node);
    }

    // Empurra no stack
    state->node_stack.push_back(node);
    state->style_stack.push_back(node->style);
    state->current_node = node;
  }
}

// ============================================================================
// Callback de fechamento de tag
// ============================================================================

void EndElementCallback(void *data, const char *name) {
  auto* state = static_cast<ParserStateWithTree*>(data);

  // === CÓDIGO LEGADO ===
  if (state->legacy_parse_data) {
    parse_end_element(state->legacy_parse_data, name);
  }

  // === NOVO: DocumentTree ===
  if (state->doc_tree) {
    FlushTextBuffer(state);  // finaliza texto pendente

    // Pop do stack
    if (!state->node_stack.empty()) {
      state->node_stack.pop_back();
    }
    if (!state->style_stack.empty()) {
      state->style_stack.pop_back();
    }

    // Atualiza current_node
    if (!state->node_stack.empty()) {
      state->current_node = state->node_stack.back();
    } else {
      state->current_node = nullptr;
    }
  }
}

// ============================================================================
// Callback de caracteres (texto)
// ============================================================================

void CharacterDataCallback(void *data, const char *txt, int len) {
  auto* state = static_cast<ParserStateWithTree*>(data);

  // === CÓDIGO LEGADO ===
  if (state->legacy_parse_data) {
    parse_character_data(state->legacy_parse_data, txt, len);
  }

  // === NOVO: DocumentTree ===
  if (state->doc_tree && state->current_node) {
    // Acumula texto em buffer temporário
    state->text_buffer.append(txt, len);

    // NOTA: Texto é finalizado em FlushTextBuffer() quando:
    // - Tag fecha
    // - Nova tag abre
    // - Fim do parsing
  }
}

// ============================================================================
// NOVO: Utilitários
// ============================================================================

content_tree::ContentNode::Type DetermineNodeType(const char* tag) {
  // Elementos de bloco
  if (!strcmp(tag, "p") || !strcmp(tag, "div") ||
      !strcmp(tag, "section") || !strcmp(tag, "article") ||
      !strcmp(tag, "blockquote")) {
    return content_tree::ContentNode::BLOCK;
  }

  // Headings
  if (!strcmp(tag, "h1") || !strcmp(tag, "h2") ||
      !strcmp(tag, "h3") || !strcmp(tag, "h4") ||
      !strcmp(tag, "h5") || !strcmp(tag, "h6")) {
    return content_tree::ContentNode::HEADING;
  }

  // Preformatado
  if (!strcmp(tag, "pre") || !strcmp(tag, "code")) {
    return content_tree::ContentNode::PREFORMATTED;
  }

  // Imagem
  if (!strcmp(tag, "img")) {
    return content_tree::ContentNode::IMAGE;
  }

  // List item
  if (!strcmp(tag, "li")) {
    return content_tree::ContentNode::LIST_ITEM;
  }

  // Table cell
  if (!strcmp(tag, "td") || !strcmp(tag, "th")) {
    return content_tree::ContentNode::TABLE_CELL;
  }

  // Inline (padrão para <span>, <em>, <strong>, <a>, etc.)
  return content_tree::ContentNode::INLINE;
}

void ApplyAttributesAndStyle(
  content_tree::ContentNode* node,
  const char* tag,
  const char** attr,
  ParserStateWithTree* state
) {
  // Aplica estilos baseados na tag
  if (!strcmp(tag, "p")) {
    node->style.margin_bottom = 12;  // espaçamento entre parágrafos
    node->style.display = 1;  // block
  } else if (!strcmp(tag, "h1")) {
    node->style.font_size = 32;
    node->style.font_weight = 700;
    node->style.margin_top = 20;
    node->style.margin_bottom = 16;
  } else if (!strcmp(tag, "em") || !strcmp(tag, "i")) {
    node->style.font_style = 1;  // italic
  } else if (!strcmp(tag, "strong") || !strcmp(tag, "b")) {
    node->style.font_weight = 700;  // bold
  }

  // Processa atributos
  for (int i = 0; attr[i]; i += 2) {
    const char* attr_name = attr[i];
    const char* attr_value = attr[i + 1];

    if (!strcmp(attr_name, "style")) {
      // Parse CSS inline: "margin-left: 40px; text-indent: 20px"
      ParseInlineCSS(attr_value, &node->style);
    } else if (!strcmp(attr_name, "href")) {
      // Link
      node->is_link = true;
      node->href_id = RegisterHref(attr_value, state);
    } else if (!strcmp(attr_name, "src") && !strcmp(tag, "img")) {
      // Imagem
      node->image_id = RegisterImage(attr_value, state);
    }
  }

  // Aplica CSS de folhas de estilo externas (se existirem)
  // Por exemplo, se há regra: p.indent { margin-left: 40px; }
  ApplyCSSRules(node, tag, attr, state);
}

void ParseInlineCSS(const char* css, content_tree::ComputedStyle* style) {
  // Exemplo simplificado
  // CSS real: "margin-left: 40px; text-indent: 20px; font-size: 14pt"

  std::string css_str(css);
  size_t pos = 0;

  while (pos < css_str.size()) {
    // Encontra próxima declaração
    size_t colon = css_str.find(':', pos);
    if (colon == std::string::npos) break;

    size_t semicolon = css_str.find(';', colon);
    if (semicolon == std::string::npos) {
      semicolon = css_str.size();
    }

    std::string property = Trim(css_str.substr(pos, colon - pos));
    std::string value = Trim(css_str.substr(colon + 1, semicolon - colon - 1));

    // Aplica propriedade
    if (property == "margin-left") {
      style->margin_left = ParsePixels(value);
    } else if (property == "margin-right") {
      style->margin_right = ParsePixels(value);
    } else if (property == "margin-top") {
      style->margin_top = ParsePixels(value);
    } else if (property == "margin-bottom") {
      style->margin_bottom = ParsePixels(value);
    } else if (property == "text-indent") {
      style->text_indent = ParsePixels(value);
    } else if (property == "font-size") {
      style->font_size = ParseFontSize(value);
    } else if (property == "color") {
      style->text_color = ParseColor(value);
    } else if (property == "text-align") {
      if (value == "center") style->text_align = 1;
      else if (value == "right") style->text_align = 2;
      else if (value == "justify") style->text_align = 3;
    }
    // ... outras propriedades

    pos = semicolon + 1;
  }
}

void FlushTextBuffer(ParserStateWithTree* state) {
  if (state->text_buffer.empty()) return;
  if (!state->current_node) return;

  // Cria nó de texto
  auto* text_node = state->doc_tree->CreateNode(content_tree::ContentNode::TEXT);
  text_node->text_utf8 = state->text_buffer;
  text_node->style = state->current_node->style;  // herda estilo do pai

  // Adiciona ao nó atual
  state->current_node->AddChild(text_node);

  // Limpa buffer
  state->text_buffer.clear();
}

// ============================================================================
// EXEMPLO: Função principal de parse
// ============================================================================

bool ParseEPUBToDocumentTree(
  const char* epub_path,
  content_tree::DocumentTree* out_tree
) {
  // Abre EPUB, extrai HTML/XHTML
  // ... código existente ...

  // Inicializa parser state
  ParserStateWithTree state;
  state.doc_tree = out_tree;
  state.current_node = nullptr;
  state.legacy_parse_data = nullptr;  // ou mantém para transição

  // Configura Expat
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, &state);
  XML_SetElementHandler(parser, StartElementCallback, EndElementCallback);
  XML_SetCharacterDataHandler(parser, CharacterDataCallback);

  // Parse do conteúdo HTML
  const char* html_content = GetHTMLContent(epub_path);
  size_t html_len = strlen(html_content);

  if (XML_Parse(parser, html_content, html_len, 1) == XML_STATUS_ERROR) {
    // Erro de parsing
    XML_ParserFree(parser);
    return false;
  }

  // Finaliza texto pendente
  FlushTextBuffer(&state);

  XML_ParserFree(parser);
  return true;
}

// ============================================================================
// VALIDAÇÃO: Comparação com sistema antigo
// ============================================================================

void ValidateDocumentTreeAgainstLegacy(
  const content_tree::DocumentTree& tree,
  const parsedata_t* legacy_data
) {
  // 1. Conta total de caracteres
  size_t tree_chars = content_tree::CountTextChars(tree.root);
  size_t legacy_chars = CountLegacyBufferChars(legacy_data);

  if (tree_chars != legacy_chars) {
    printf("WARNING: Character count mismatch! Tree=%zu Legacy=%zu\n",
           tree_chars, legacy_chars);
  } else {
    printf("OK: Character counts match (%zu)\n", tree_chars);
  }

  // 2. Verifica preservação de estilos
  // ... verificar se margens, indentações, etc. estão corretas ...

  // 3. Debug: imprime árvore
  printf("=== Document Tree ===\n");
  printf("%s\n", content_tree::DebugPrintTree(tree.root).c_str());
}

// ============================================================================
// EXEMPLO DE USO FINAL
// ============================================================================

void TestNewArchitecture() {
  // Cria árvore
  content_tree::DocumentTree tree;

  // Parse EPUB para árvore
  bool success = ParseEPUBToDocumentTree("test.epub", &tree);
  if (!success) {
    printf("Parse failed!\n");
    return;
  }

  // Debug: imprime árvore
  printf("Parsed %zu nodes, %zu text chars\n",
         tree.all_nodes.size(),
         content_tree::CountTextChars(tree.root));

  printf("\nDocument structure:\n");
  printf("%s\n", content_tree::DebugPrintTree(tree.root).c_str());

  // Exemplo de bookmark
  content_tree::DocumentTree::Bookmark bm = tree.FindNodeByGlobalOffset(1000);
  if (bm.IsValid()) {
    printf("\nBookmark at offset 1000:\n");
    printf("  Node: %s\n", bm.node->tag_name.c_str());
    printf("  Char offset in node: %zu\n", bm.char_offset_in_node);
  }

  // ===== PRÓXIMA FASE: Layout Engine =====
  // LayoutEngine engine;
  // LayoutMetrics metrics = GetCurrentMetrics();
  // auto page0 = engine.ComputePage(tree.root, 0, metrics);
  // RenderPage(page0);
}

// ============================================================================
// NOTAS IMPORTANTES
// ============================================================================

/*

MIGRAÇÃO GRADUAL:
-----------------

1. Adicionar flag de compilação:
   #define USE_DOCUMENT_TREE 1

2. Durante transição, manter ambos sistemas:
   - legacy_parse_data → sistema antigo (buffer de tokens)
   - doc_tree → novo sistema

3. Comparar resultados:
   - Mesmo número de caracteres?
   - Mesmos estilos aplicados?
   - Mesmas quebras de linha? (após implementar layout engine)

4. Quando validado, remover código legado


VANTAGENS DESTA ABORDAGEM:
---------------------------

✅ Contexto CSS preservado em cada ContentNode
✅ Pode recalcular layout sem re-parse
✅ Bookmarks estáveis (apontam para nodes)
✅ Fácil de debugar (inspeciona árvore)
✅ Extensível (adicionar novos tipos de nós)


DESVANTAGENS/RISCOS:
--------------------

⚠️  Uso de memória maior (árvore completa vs. buffer linear)
    - Mitigação: Liberar árvore quando não estiver lendo

⚠️  Performance do layout pode ser mais lenta inicialmente
    - Mitigação: Cache de páginas, otimizações

⚠️  Mudança grande no código (risco de bugs)
    - Mitigação: Migração gradual, testes extensivos


PRÓXIMOS PASSOS:
----------------

1. Implementar ContentNode e DocumentTree headers
2. Modificar book_xml_parser.cpp para construir árvore
3. Validar contra sistema antigo (comparação)
4. Benchmark de memória e performance
5. Decisão: continuar ou ajustar?

*/
