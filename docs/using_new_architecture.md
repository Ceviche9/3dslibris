# Como Usar a Nova Arquitetura

## Compilação

Os novos arquivos foram adicionados ao Makefile:
- `source/book/content_node.cpp`
- `source/book/layout_engine.cpp`
- `source/book/page_cache.cpp`
- `source/book/css_parser.cpp`
- `source/book/document_tree_parser.cpp`

Compile normalmente com:
```bash
make
```

## Uso Básico

### 1. Parse EPUB para DocumentTree

```cpp
#include "book/content_node.h"
#include "book/document_tree_parser.h"
#include <expat.h>

// Criar árvore de documento
content_tree::DocumentTree tree;

// Inicializar parser state
document_tree_parser::TreeParserState parser_state;
document_tree_parser::InitTreeParserState(&parser_state, &tree);

// Configurar Expat XML parser
XML_Parser xml_parser = XML_ParserCreate(NULL);
XML_SetUserData(xml_parser, &parser_state);

// Callbacks que constroem DocumentTree
XML_SetElementHandler(xml_parser,
  [](void* data, const char* name, const char** attr) {
    auto* state = static_cast<document_tree_parser::TreeParserState*>(data);
    document_tree_parser::HandleStartElement(state, name, attr);
  },
  [](void* data, const char* name) {
    auto* state = static_cast<document_tree_parser::TreeParserState*>(data);
    document_tree_parser::HandleEndElement(state, name);
  }
);

XML_SetCharacterDataHandler(xml_parser,
  [](void* data, const char* txt, int len) {
    auto* state = static_cast<document_tree_parser::TreeParserState*>(data);
    document_tree_parser::HandleCharacterData(state, txt, len);
  }
);

// Parse HTML/XML content
const char* html_content = LoadHTMLContent();
size_t html_len = strlen(html_content);

if (XML_Parse(xml_parser, html_content, html_len, 1) == XML_STATUS_OK) {
  // Sucesso!
  // Finaliza texto pendente
  document_tree_parser::FlushTextBuffer(&parser_state);
}

XML_ParserFree(xml_parser);

// Agora tree.root contém toda estrutura do documento
printf("Parsed %zu nodes\n", tree.all_nodes.size());
printf("Total text chars: %zu\n", content_tree::CountTextChars(tree.root));
```

### 2. Calcular Layout de Páginas

```cpp
#include "book/layout_engine.h"
#include "ui/text.h"

// Configurar métricas de layout
layout_engine::LayoutMetrics metrics;
metrics.screen_width = 400;
metrics.screen_height = 240;
metrics.base_margin_left = 16;
metrics.base_margin_right = 16;
metrics.base_margin_top = 12;
metrics.base_margin_bottom = 36;
metrics.line_spacing = 3;
metrics.space_advance = 6; // largura do espaço

// Função de medição (reutiliza sistema existente)
Text* text_renderer = GetTextRenderer();
metrics.measure_fn = [](uint32_t cp, void* ctx) -> int {
  Text* ts = static_cast<Text*>(ctx);
  return ts->GetGlyphAdvance(cp);
};
metrics.measure_ctx = text_renderer;

// Criar layout engine
layout_engine::LayoutEngine engine;

// Calcular primeira página (começa do início do documento)
layout_engine::PageStart start(tree.root, 0);
auto page0 = engine.ComputePage(start, metrics);

printf("Page 0 has %zu lines\n", page0.lines.size());

// Calcular próxima página (retoma de onde página 0 parou)
auto page1 = engine.ComputePage(page0.end_position, metrics);

printf("Page 1 has %zu lines\n", page1.lines.size());
```

### 3. Renderizar Página Calculada

```cpp
void RenderLayoutPage(const layout_engine::LayoutPage& page, Text* text) {
  // Percorre todas as linhas da página
  for (const auto& line : page.lines) {
    // Percorre fragmentos de texto na linha
    for (const auto& frag : line.fragments) {
      // Posiciona pen
      text->SetPen(frag.x, frag.y);
      text->SetColor(frag.color);

      // Desenha glyphs
      int x = frag.x;
      for (const auto& glyph : frag.glyphs) {
        text->DrawGlyph(glyph.text.codepoint, x, frag.y);
        x += glyph.advance;
      }

      // Aplica link se necessário
      if (frag.is_link) {
        // Desenha sublinhado, registra área clicável, etc.
      }
    }
  }
}
```

### 4. Usar PageCache para Performance

```cpp
#include "book/page_cache.h"

// Criar cache
page_cache::PageCache cache;
cache.SetMaxSize(5); // cache últimas 5 páginas

layout_engine::LayoutEngine engine;
layout_engine::LayoutMetrics metrics = GetCurrentMetrics();

// Obter página (calcula ou busca do cache)
auto& page = cache.GetPage(0, tree, metrics, engine);
RenderLayoutPage(page, text_renderer);

// Usuário muda tamanho de fonte
metrics.base_margin_left = 20; // margens maiores
cache.InvalidateAll(); // invalida cache

// Próxima chamada irá recalcular com novas métricas
auto& new_page = cache.GetPage(0, tree, metrics, engine);
```

## Bookmarks

Bookmarks apontam para nós da árvore, não offsets:

```cpp
// Criar bookmark no offset global 1000
auto bookmark = tree.FindNodeByGlobalOffset(1000);
tree.bookmarks.push_back({
  bookmark.node,
  bookmark.char_offset_in_node,
  "Chapter 3"
});

// Retomar leitura do bookmark
layout_engine::PageStart start(
  tree.bookmarks[0].node,
  tree.bookmarks[0].char_offset_in_node
);
auto page = engine.ComputePage(start, metrics);
```

## Debug

```cpp
// Imprimir estrutura da árvore
std::string debug = content_tree::DebugPrintTree(tree.root);
printf("%s\n", debug.c_str());

// Output:
// BLOCK <body>
//   BLOCK <p> margin-bottom=12
//     TEXT "Este é um parágrafo de texto..."
//   HEADING <h1> margin-top=20 margin-bottom=16
//     TEXT "Capítulo 1"
//   BLOCK <p> margin-left=40 text-indent=20
//     TEXT "Parágrafo com indentação..."
```

## Integração com Sistema Legado

Durante a fase de transição, você pode manter ambos os sistemas:

```cpp
#ifdef USE_NEW_LAYOUT_ENGINE
  // Novo sistema
  content_tree::DocumentTree tree;
  ParseToDocumentTree(epub_path, &tree);
  layout_engine::LayoutEngine engine;
  auto page = engine.ComputePage(start, metrics);
  RenderLayoutPage(page, text_renderer);
#else
  // Sistema antigo (atual)
  parsedata_t parse_data;
  parse_xml(epub_path, &parse_data);
  Page* page = GetCurrentPage();
  page->Draw(text_renderer);
#endif
```

## Próximos Passos

1. **RTL Support**: Integrar BiDi existente com layout engine
2. **Tabelas**: Implementar layout de tabelas
3. **Imagens**: Suporte a imagens inline
4. **Hifenização**: Adicionar quebra de palavras correta
5. **Performance**: Otimizações de cache de glyphs shaped

## Vantagens da Nova Arquitetura

✅ **Contexto preservado**: Margens, indentação funcionam entre páginas
✅ **Recalculável**: Mudar fonte/margens não precisa re-parse
✅ **Bookmarks estáveis**: Apontam para nós, não mudam com layout
✅ **Debug fácil**: Pode inspecionar árvore de documento
✅ **Extensível**: Adicionar features sem quebrar código existente
