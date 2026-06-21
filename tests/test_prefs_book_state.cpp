#include "settings/prefs_book_state.h"

#include <stdio.h>

static int failures = 0;

static void Expect(bool condition, const char *message) {
  if (condition)
    return;
  fprintf(stderr, "FAIL: %s\n", message);
  failures++;
}

int main() {
  SavedBookStateMap states;

  SavedBookState nested;
  nested.position = 80;
  nested.bookmarks.push_back(12);
  RememberSavedBookState(&states, "sdmc:/books/series", "book.epub",
                         nested);

  SavedBookState root;
  root.position = 4;
  RememberSavedBookState(&states, "sdmc:/books", "other.epub", root);

  const SavedBookState *restored = FindSavedBookState(
      states, "sdmc:/books/series", "book.epub");
  Expect(restored != NULL, "nested book survives a root-folder update");
  Expect(restored && restored->position == 80,
         "nested book keeps its saved page");
  Expect(restored && restored->bookmarks.size() == 1 &&
             restored->bookmarks[0] == 12,
         "nested book keeps its bookmarks");

  if (failures == 0)
    printf("prefs book state tests passed\n");
  return failures == 0 ? 0 : 1;
}
