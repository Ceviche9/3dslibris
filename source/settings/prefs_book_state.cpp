#include "settings/prefs_book_state.h"

std::string MakeSavedBookKey(const char *folder, const char *filename) {
  return std::string(folder ? folder : "") + "\n" +
         std::string(filename ? filename : "");
}

bool SplitSavedBookKey(const std::string &key, std::string *folder,
                       std::string *filename) {
  const size_t separator = key.find('\n');
  if (separator == std::string::npos || !folder || !filename)
    return false;
  *folder = key.substr(0, separator);
  *filename = key.substr(separator + 1);
  return !filename->empty();
}

void RememberSavedBookState(SavedBookStateMap *states, const char *folder,
                            const char *filename,
                            const SavedBookState &state) {
  if (!states || !filename || !filename[0])
    return;
  (*states)[MakeSavedBookKey(folder, filename)] = state;
}

SavedBookState *FindSavedBookState(SavedBookStateMap *states,
                                  const char *folder, const char *filename) {
  if (!states)
    return NULL;
  SavedBookStateMap::iterator it =
      states->find(MakeSavedBookKey(folder, filename));
  return it == states->end() ? NULL : &it->second;
}

const SavedBookState *FindSavedBookState(const SavedBookStateMap &states,
                                        const char *folder,
                                        const char *filename) {
  SavedBookStateMap::const_iterator it =
      states.find(MakeSavedBookKey(folder, filename));
  return it == states.end() ? NULL : &it->second;
}
