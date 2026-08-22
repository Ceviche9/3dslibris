#pragma once
#include "formats/epub/epub.h"
#include "book/book_parse_deps.h"
#include "book/epub_css_class_map.h"
#include "expat.h"
#include "minizip/unzip.h"
#include <3ds.h>
#include <string>
#include <vector>
typedef BookParseDeps EpubDeps;

class IStatusReporter;

void epub_data_init(epub_data_t *d);
void epub_data_delete(epub_data_t *d);
void epub_container_start(void *data, const char *el, const char **attr);
void epub_rootfile_start(void *data, const char *el, const char **attr);
void epub_rootfile_end(void *data, const char *el);
void epub_rootfile_char(void *data, const XML_Char *txt, int len);
int epub_parse_currentfile(unzFile uf, epub_data_t *epd, const EpubDeps &deps,
                           unzFile css_scan_uf = NULL);
int LoadEpubPackageData(unzFile uf, Book *book, epub_data_t *parsedata,
                        std::string *opf_folder, const EpubDeps &deps);
int LoadEpubPackageForParse(unzFile uf, Book *book, epub_data_t *parsedata,
                            std::string *folder, bool metadataonly,
                            const EpubDeps &deps
#ifdef DSLIBRIS_DEBUG
                            , u64 *t_after_container, u64 *t_after_rootfile
#endif
);
void ApplyEpubMetadataOnlyResult(Book *book, epub_data_t &parsedata,
                                 const std::string &folder);
std::vector<std::string> BuildEpubSpineDocumentList(const epub_data_t &parsedata);

// Resolves and parses the CSS stylesheet(s) linked from xhtml_path's <head>
// (via <link rel="stylesheet">) into *out. Results are cached per-doc and
// per-css-path on epd (epd->css_href_by_doc / css_class_map_by_path) so a
// stylesheet shared across the whole spine is only parsed once. Pass an
// already-open external_scan_uf to avoid reopening the zip per document;
// pass NULL to have this function open/close its own handle.
void LoadCssClassMapForDoc(const std::string &archive_path,
                           const std::string &xhtml_path,
                           IStatusReporter *reporter,
                           epub_data_t *epd,
                           epub_css_class_map::CssClassMap *out,
                           unzFile external_scan_uf = NULL);
