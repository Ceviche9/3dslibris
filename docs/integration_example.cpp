// integration_example.cpp
// Exemplo prático de como integrar nova arquitetura com Book class

#include "book/book.h"
#include "book/content_node.h"
#include "book/document_tree_parser.h"
#include "book/layout_engine.h"
#include "book/page_cache.h"
#include "book/layout_page_renderer.h"
#include <expat.h>

// ============================================================================
// PASSO 1: Modificar Book Class (book.h)
// ============================================================================

class Book {
private:
  // Sistema ANTIGO (manter temporariamente)
  #ifndef USE_NEW_LAYOUT_ENGINE
    std::vector<Page*> pages_;
  #endif

  // Sistema NOVO
  #ifdef USE_NEW_LAYOUT_ENGINE
    content_tree::DocumentTree* doc_tree_;
    page_cache::PageCache page_cache_;
    layout_engine::LayoutEngine layout_engine_;
    int current_page_number_;
  #endif

  // ... resto do código existente ...

public:
  Book();
  ~Book();

  #ifdef USE_NEW_LAYOUT_ENGINE
    bool ParseEPUBToDocumentTree(const char* epub_path);
    void RenderPage(int page_num, Text* text);
    void NextPage();
    void PrevPage();
    int GetTotalPages(); // estimativa
  #endif
};

// ============================================================================
// PASSO 2: Implementar Parsing (book.cpp)
// ============================================================================

#ifdef USE_NEW_LAYOUT_ENGINE

Book::Book()
  : doc_tree_(nullptr),
    current_page_number_(0) {

  doc_tree_ = new content_tree::DocumentTree();
  page_cache_.SetMaxSize(5);
}

Book::~Book() {
  delete doc_tree_;
}

// Callback para Expat XML parser
static void XMLCALL StartElementHandler(void* data, const char* name, const char** attr) {
  auto* state = static_cast<document_tree_parser::TreeParserState*>(data);
  document_tree_parser::HandleStartElement(state, name, attr);
}

static void XMLCALL EndElementHandler(void* data, const char* name) {
  auto* state = static_cast<document_tree_parser::TreeParserState*>(data);
  document_tree_parser::HandleEndElement(state, name);
}

static void XMLCALL CharacterDataHandler(void* data, const char* txt, int len) {
  auto* state = static_cast<document_tree_parser::TreeParserState*>(data);
  document_tree_parser::HandleCharacterData(state, txt, len);
}

bool Book::ParseEPUBToDocumentTree(const char* epub_path) {
  // 1. Extrair HTML do EPUB (código existente)
  // ... código de extração do ZIP ...
  const char* html_content = GetHTMLContentFromEPUB(epub_path);
  if (!html_content) return false;

  // 2. Inicializar parser state
  document_tree_parser::TreeParserState parser_state;
  document_tree_parser::InitTreeParserState(&parser_state, doc_tree_);

  // 3. Configurar Expat
  XML_Parser xml_parser = XML_ParserCreate(NULL);
  XML_SetUserData(xml_parser, &parser_state);
  XML_SetElementHandler(xml_parser, StartElementHandler, EndElementHandler);
  XML_SetCharacterDataHandler(xml_parser, CharacterDataHandler);

  // 4. Parse HTML
  size_t html_len = strlen(html_content);
  bool success = (XML_Parse(xml_parser, html_content, html_len, 1) == XML_STATUS_OK);

  if (success) {
    document_tree_parser::FlushTextBuffer(&parser_state);
  }

  XML_ParserFree(xml_parser);

  printf("Parsed %zu nodes, %zu text chars\n",
         doc_tree_->all_nodes.size(),
         content_tree::CountTextChars(doc_tree_->root));

  return success;
}

void Book::RenderPage(int page_num, Text* text) {
  if (!doc_tree_ || !text) return;

  // Configurar métricas de layout
  layout_engine::LayoutMetrics metrics;
  metrics.screen_width = 400;
  metrics.screen_height = 240;
  metrics.base_margin_left = MARGINLEFT;
  metrics.base_margin_right = MARGINRIGHT;
  metrics.base_margin_top = MARGINTOP;
  metrics.base_margin_bottom = MARGINBOTTOM;
  metrics.line_spacing = LINESPACING;
  metrics.space_advance = 6; // largura aproximada do espaço

  // Função de medição
  metrics.measure_fn = [](uint32_t cp, void* ctx) -> int {
    Text* ts = static_cast<Text*>(ctx);
    return ts->GetGlyphAdvance(cp);
  };
  metrics.measure_ctx = text;

  // Obter página do cache (ou calcular se não estiver)
  auto& page = page_cache_.GetPage(page_num, *doc_tree_, metrics, layout_engine_);

  // Renderizar
  layout_page_renderer::RenderPage(page, text);

  current_page_number_ = page_num;
}

void Book::NextPage() {
  current_page_number_++;
  // Limitar ao máximo de páginas (estimado)
}

void Book::PrevPage() {
  if (current_page_number_ > 0) {
    current_page_number_--;
  }
}

int Book::GetTotalPages() {
  // Estimativa baseada em contagem de caracteres
  size_t total_chars = content_tree::CountTextChars(doc_tree_->root);
  int chars_per_page = 1000; // estimativa
  return (total_chars / chars_per_page) + 1;
}

#endif // USE_NEW_LAYOUT_ENGINE

// ============================================================================
// PASSO 3: Uso no Reader
// ============================================================================

#ifdef USE_NEW_LAYOUT_ENGINE

void ReaderScreen::Render() {
  if (!book_) return;

  // Limpar tela
  text_->Clear();

  // Renderizar página atual
  book_->RenderPage(current_page_, text_);

  // Mostrar número da página
  char page_info[64];
  snprintf(page_info, sizeof(page_info), "Página %d/%d",
           current_page_ + 1, book_->GetTotalPages());
  text_->DrawString(10, 220, page_info);

  // Flush para tela
  text_->Flush();
}

void ReaderScreen::HandleInput(u32 keys_down) {
  if (keys_down & KEY_DRIGHT) {
    book_->NextPage();
  } else if (keys_down & KEY_DLEFT) {
    book_->PrevPage();
  }
}

#endif

// ============================================================================
// PASSO 4: Compilação Condicional
// ============================================================================

/*

No Makefile, adicionar flag:

USE_NEW_LAYOUT_ENGINE ?= 0

ifeq ($(USE_NEW_LAYOUT_ENGINE),1)
  CXXFLAGS += -DUSE_NEW_LAYOUT_ENGINE=1
endif

Compilar com novo sistema:
  make clean
  USE_NEW_LAYOUT_ENGINE=1 make

Compilar com sistema antigo:
  make clean
  make

*/

// ============================================================================
// PASSO 5: Migração Gradual
// ============================================================================

/*

FASE 1: Testes Iniciais
  - Compilar com novo sistema
  - Testar com EPUB simples
  - Validar que texto renderiza corretamente
  - Comparar visualmente com sistema antigo

FASE 2: Validação
  - Testar com EPUBs complexos
  - Verificar performance (tempo de parse, memória)
  - Testar navegação (avançar/voltar páginas)
  - Verificar margens e indentação

FASE 3: Bookmarks
  - Migrar sistema de bookmarks para usar ContentNode*
  - Testar que bookmarks sobrevivem mudança de fonte

FASE 4: Features Avançadas
  - Hifenização (já implementada!)
  - Tabelas (se necessário)
  - Imagens inline (se necessário)

FASE 5: Transição Completa
  - Remover código antigo (#ifdef)
  - Limpar arquivos obsoletos
  - Atualizar documentação

*/

// ============================================================================
// EXEMPLO COMPLETO: Main Reader Loop
// ============================================================================

#ifdef USE_NEW_LAYOUT_ENGINE

int main(int argc, char** argv) {
  // Inicializar 3DS
  gfxInitDefault();
  // ...

  // Criar book
  Book* book = new Book();

  // Parse EPUB
  if (!book->ParseEPUBToDocumentTree("sdmc:/3ds/3dslibris/books/livro.epub")) {
    printf("Erro ao abrir livro!\n");
    return 1;
  }

  // Criar renderer de texto
  Text* text = new Text();

  // Loop principal
  bool running = true;
  while (running && aptMainLoop()) {
    hidScanInput();
    u32 keys_down = hidKeysDown();

    if (keys_down & KEY_START) {
      running = false;
    }

    // Navegação
    if (keys_down & KEY_DRIGHT) {
      book->NextPage();
    } else if (keys_down & KEY_DLEFT) {
      book->PrevPage();
    }

    // Renderizar página atual
    text->Clear();
    book->RenderPage(book->GetCurrentPageNumber(), text);
    text->Flush();

    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
  }

  delete text;
  delete book;
  gfxExit();
  return 0;
}

#endif

// ============================================================================
// DIFERENÇAS PRÁTICAS
// ============================================================================

/*

SISTEMA ANTIGO:
===============

Parse EPUB:
  1. Extrai HTML
  2. Expat callbacks →  EmitFlowedShapedText()
  3. Emite tokens para Page.buf[]
  4. Problemas:
     - Margens perdidas entre páginas
     - TEXT_LINE_START_X cortava texto
     - Indentação desabilitada

Renderizar:
  1. Page::Draw() lê buf[]
  2. Interpreta tokens
  3. Desenha glyphs

Mudar fonte:
  ❌ Precisa RE-PARSE completo do EPUB


SISTEMA NOVO:
==============

Parse EPUB:
  1. Extrai HTML
  2. Expat callbacks → DocumentTree (ContentNode hierarchy)
  3. Preserva TUDO: margens, indentação, estilos
  4. Vantagens:
     ✅ Contexto preservado
     ✅ Pode inspecionar árvore

Renderizar:
  1. LayoutEngine.ComputePage() → LayoutPage
  2. layout_page_renderer.RenderPage()
  3. Desenha glyphs em posições corretas

Mudar fonte:
  ✅ Invalida PageCache, recalcula layout
  ✅ DocumentTree INTACTA (não re-parse!)


EXEMPLO PRÁTICO:
================

HTML:
  <p style="margin-left: 40px; text-indent: 20px">
    Este é um parágrafo muito longo que quebra em várias páginas
    e o sistema antigo perdia o contexto de margin-left.
  </p>

Sistema Antigo (BUGGY):
  Página 1: x = 16 + 40 + 20 = 76 ✅
  Página 2: x = 16 (perdeu contexto!) ❌ TEXTO CORTADO!

Sistema Novo (CORRETO):
  Página 1:
    ContentNode{margin_left:40, text_indent:20}
    block_stack=[{effective_margin_left: 76}]
    Fragmentos em x=76 ✅

  Página 2:
    Retoma do MESMO ContentNode
    block_stack=[{effective_margin_left: 76}] ← PRESERVADO!
    Fragmentos em x=76 ✅

*/
