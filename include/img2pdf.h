#ifndef IMG2PDF_H
#define IMG2PDF_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    wchar_t full_path[MAX_PATH * 2];
    wchar_t file_name[MAX_PATH];
    uint64_t file_size;
    uint32_t width;
    uint32_t height;
    int page_num;
} Img2PdfItem;

typedef struct {
    Img2PdfItem *items;
    size_t count;
    size_t capacity;
} Img2PdfItemList;

/* Create and manage Img2PdfItemList */
Img2PdfItemList* img2pdf_create_list(void);
void img2pdf_free_list(Img2PdfItemList *list);
void img2pdf_clear_list(Img2PdfItemList *list);
bool img2pdf_add_item(Img2PdfItemList *list, const wchar_t *full_path, const wchar_t *file_name, uint64_t size);

/* Check if file extension is a supported image (.jpg, .jpeg, .png, .bmp, .gif, .tif, .tiff, .webp, .jp2, .j2k, .jpx, .jpm) */
bool img2pdf_is_supported_image(const wchar_t *filename);

/* Scan folder for image files, sorted naturally by name (1, 2, 10 / 01, 02).
 * If recursive is true, traverses child directories. */
bool img2pdf_scan_folder(Img2PdfItemList *list, const wchar_t *folder_path, bool recursive);

/* Convert image list into a single PDF document */
bool img2pdf_convert_to_pdf(
    const Img2PdfItemList *list,
    const wchar_t *output_pdf_path,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
);

#ifdef __cplusplus
}
#endif

#endif /* IMG2PDF_H */
