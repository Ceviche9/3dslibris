# Bug Fixes - Telas Brancas e Crash

**Data:** 2026-08-21
**Versão:** Após primeira integração da nova arquitetura

## Problema Reportado

- **Sintoma 1**: Todas as páginas dos livros aparecem em branco
- **Sintoma 2**: Aplicação crasha ao fechar, reiniciando o 3DS
- **Log**: `error (1)` aparece 3 vezes no log

## Bugs Encontrados e Corrigidos

### 1. Parser: Elementos HTML Estruturais Não Reconhecidos como BLOCK

**Arquivo**: `source/book/document_tree_parser.cpp`

**Problema**:
Os elementos estruturais HTML (`<html>`, `<head>`, `<body>`) não estavam na lista de elementos BLOCK, sendo tratados como INLINE. Isso causava problemas no layout engine que precisa processar blocos corretamente.

**Solução**:
```cpp
// Adicionado html, head, body, e outros elementos estruturais
if (strcmp(tag, "html") == 0 || strcmp(tag, "head") == 0 ||
    strcmp(tag, "body") == 0) {
  return content_tree::ContentNode::BLOCK;
}

// Adicionados mais elementos block
if (strcmp(tag, "header") == 0 || strcmp(tag, "footer") == 0 ||
    strcmp(tag, "aside") == 0 || strcmp(tag, "nav") == 0 ||
    strcmp(tag, "main") == 0 || strcmp(tag, "ul") == 0 ||
    strcmp(tag, "ol") == 0 || strcmp(tag, "dl") == 0 ||
    strcmp(tag, "table") == 0) {
  return content_tree::ContentNode::BLOCK;
}
```

### 2. Layout Engine: Elementos Não-Visuais Sendo Renderizados

**Arquivo**: `source/book/layout_engine.cpp`

**Problema**:
Elementos como `<head>`, `<script>`, `<style>`, `<meta>`, `<link>`, `<title>` estavam sendo processados pelo layout engine, potencialmente causando conteúdo invisível ou problemas de layout.

**Solução**:
```cpp
// Skip non-visual elements
if (!node->tag_name.empty()) {
  if (node->tag_name == "head" || node->tag_name == "script" ||
      node->tag_name == "style" || node->tag_name == "meta" ||
      node->tag_name == "link" || node->tag_name == "title") {
    return;
  }
}
```

### 3. Renderização: Posicionamento Incorreto de Fragmentos

**Arquivo**: `source/book/layout_page_renderer.cpp`

**Problema 1**: A renderização estava setando a caneta X para 0 no início de cada linha, ignorando a posição X calculada pelo layout engine para cada fragmento.

**Problema 2**: A renderização não estava usando a posição Y do fragmento.

**Solução**:
```cpp
void RenderFragment(
  const layout_engine::LineFragment& fragment,
  Text* text,
  Book* book
) {
  if (!text || !fragment.source_node) return;
  if (fragment.glyphs.empty()) return;

  // Set pen position to fragment start - CRÍTICO!
  text->SetPen(fragment.x, fragment.y);

  // Set text color
  text->fgcolor = fragment.color;
  text->usefgcolor = (fragment.color != 0);

  // Render each glyph
  for (const auto& glyph : fragment.glyphs) {
    text->PrintChar(glyph.text.codepoint);
  }
}
```

### 4. Renderização: Falta de Inicialização Correta das Telas

**Arquivo**: `source/book/layout_page_renderer.cpp`

**Problema**:
A renderização não estava configurando corretamente qual tela (left/right) deveria ser a primeira com base na orientação do dispositivo.

**Solução**:
```cpp
const unsigned char orientation = book->GetOrientation();
const bool first_screen_is_left = orientation_utils::FirstScreenIsLeft(orientation);
u16 *first_screen = first_screen_is_left ? text->screenleft : text->screenright;

// Clear both screens
text->SetScreen(text->screenleft);
book->DrawTopGradientBackground();
text->MarkScreenDirty(text->screenleft);

text->SetScreen(text->screenright);
book->DrawBottomGradientBackground();
text->MarkScreenDirty(text->screenright);

// Start rendering on first screen
text->SetScreen(first_screen);
text->InitPen();
```

## Análise do Crash

O crash ao fechar a aplicação pode ter sido causado por:

1. **Acesso a memória inválida**: Se o layout engine tentou acessar nodes inválidos
2. **Double-free**: Se a DocumentTree foi deletada incorretamente
3. **Stack overflow**: Se a recursão do layout foi muito profunda

**Nota**: Com os bugs de renderização corrigidos, o crash pode ter sido resolvido como efeito colateral, já que a aplicação não estava mais tentando renderizar conteúdo com posições inválidas.

## Teste Recomendado

1. Instalar o novo `3dslibris.cia`
2. Abrir um livro EPUB
3. Verificar se o conteúdo aparece corretamente
4. Navegar entre páginas
5. Fechar o aplicativo e verificar se não há crash

## Arquivos Modificados

1. `source/book/document_tree_parser.cpp` - Parser reconhece elementos estruturais
2. `source/book/layout_engine.cpp` - Skip de elementos não-visuais
3. `source/book/layout_page_renderer.cpp` - Posicionamento correto e inicialização

## Build Status

```bash
$ make -j4
built ... 3dslibris.smdh
linking 3dslibris.elf
built ... 3dslibris.3dsx

$ make cia
built ... 3dslibris.cia
```

✅ Sem erros de compilação
✅ CIA gerado com sucesso

## Próximos Passos

- Testar no hardware real
- Verificar se crash foi resolvido
- Verificar navegação entre páginas
- Verificar diferentes tipos de EPUB
