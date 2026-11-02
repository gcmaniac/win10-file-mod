#include "img2pdf.h"
#include <stdio.h>
#include <assert.h>

static void test_cb(const wchar_t *file, int cur, int total, const wchar_t *msg, void *user_data) {
    (void)user_data;
    wprintf(L"Progress [%d/%d]: %ls - %ls\n", cur, total, file, msg);
}

int wmain(void) {
    wprintf(L"--- Test Image to PDF ---\n");

    Img2PdfItemList *list = img2pdf_create_list();
    assert(list != NULL);

    /* Test is_supported_image */
    assert(img2pdf_is_supported_image(L"test.jpg"));
    assert(img2pdf_is_supported_image(L"PHOTO.PNG"));
    assert(img2pdf_is_supported_image(L"scan.bmp"));
    assert(img2pdf_is_supported_image(L"doc.tiff"));
    assert(img2pdf_is_supported_image(L"archive.jp2"));
    assert(img2pdf_is_supported_image(L"book.j2k"));
    assert(!img2pdf_is_supported_image(L"file.txt"));
    assert(!img2pdf_is_supported_image(L"audio.mp3"));

    /* Scan assets folder */
    bool scan_ok = img2pdf_scan_folder(list, L"assets", false);
    wprintf(L"Scan assets folder result: %d, found %zu images\n", scan_ok, list->count);
    assert(scan_ok);
    assert(list->count >= 2);

    for (size_t i = 0; i < list->count; i++) {
        wprintf(L"Page %d: %ls (%u x %u, %llu bytes)\n",
            list->items[i].page_num,
            list->items[i].file_name,
            list->items[i].width,
            list->items[i].height,
            (unsigned long long)list->items[i].file_size);
    }

    /* Convert to PDF */
    const wchar_t *out_pdf = L"tests\\test_combined.pdf";
    bool conv_ok = img2pdf_convert_to_pdf(list, out_pdf, test_cb, NULL, NULL);
    wprintf(L"Convert to PDF result: %d\n", conv_ok);
    assert(conv_ok);

    /* Check output file exists and is > 0 */
    FILE *fp = _wfopen(out_pdf, L"rb");
    assert(fp != NULL);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    wprintf(L"Generated PDF size: %ld bytes\n", size);
    assert(size > 1000);

    /* Test scanning the user folder D:\\downloads\\Ushul min Ilmil Ushul */
    const wchar_t *ushul_folder = L"D:\\downloads\\Ushul min Ilmil Ushul";
    if (GetFileAttributesW(ushul_folder) != INVALID_FILE_ATTRIBUTES) {
        wprintf(L"\n--- Testing User Folder: %ls ---\n", ushul_folder);
        bool u_ok = img2pdf_scan_folder(list, ushul_folder, true);
        wprintf(L"Scan result: %d, Found %zu images\n", u_ok, list->count);
        assert(u_ok);
        assert(list->count == 72);
        wprintf(L"First item: %ls (%u x %u)\n", list->items[0].file_name, list->items[0].width, list->items[0].height);
        assert(list->items[0].width == 2144);
        assert(list->items[0].height == 3024);
    }

    img2pdf_free_list(list);
    wprintf(L"ALL TESTS PASSED!\n");
    return 0;
}
