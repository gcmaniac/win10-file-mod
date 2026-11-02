#include "img2pdf.h"
#include <ole2.h>
#include <shlwapi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GDI+ flat API dynamic prototypes and structures */
typedef struct {
    UINT32 GdiplusVersion;
    void *DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
} GpStartupInput;

typedef struct {
    GUID Guid;
    ULONG NumberOfValues;
    ULONG Type;
    VOID *Value;
} GpEncoderParameter;

typedef struct {
    UINT Count;
    GpEncoderParameter Parameter[1];
} GpEncoderParameters;

typedef int (WINAPI *FnGdiplusStartup)(ULONG_PTR*, const GpStartupInput*, void*);
typedef void (WINAPI *FnGdiplusShutdown)(ULONG_PTR);
typedef int (WINAPI *FnGdipLoadImageFromFile)(const WCHAR*, void**);
typedef int (WINAPI *FnGdipGetImageWidth)(void*, UINT*);
typedef int (WINAPI *FnGdipGetImageHeight)(void*, UINT*);
typedef int (WINAPI *FnGdipSaveImageToStream)(void*, IStream*, const CLSID*, const void*);
typedef int (WINAPI *FnGdipDisposeImage)(void*);

static const CLSID CLSID_JpegEncoder = {
    0x557cf401, 0x1a04, 0x11d3,
    { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e }
};

static const GUID GUID_EncoderQuality = {
    0x1d5be4b5, 0xfa4a, 0x452d,
    { 0x9c, 0xdd, 0x5d, 0xb3, 0x51, 0x05, 0xe7, 0xeb }
};

typedef struct {
    HMODULE hLib;
    ULONG_PTR token;
    FnGdiplusStartup Startup;
    FnGdiplusShutdown Shutdown;
    FnGdipLoadImageFromFile LoadImage;
    FnGdipGetImageWidth GetWidth;
    FnGdipGetImageHeight GetHeight;
    FnGdipSaveImageToStream SaveToStream;
    FnGdipDisposeImage DisposeImage;
} GdiContext;

static bool gdi_init(GdiContext *ctx) {
    if (!ctx) return false;
    memset(ctx, 0, sizeof(GdiContext));

    ctx->hLib = LoadLibraryW(L"gdiplus.dll");
    if (!ctx->hLib) return false;

    ctx->Startup = (FnGdiplusStartup)GetProcAddress(ctx->hLib, "GdiplusStartup");
    ctx->Shutdown = (FnGdiplusShutdown)GetProcAddress(ctx->hLib, "GdiplusShutdown");
    ctx->LoadImage = (FnGdipLoadImageFromFile)GetProcAddress(ctx->hLib, "GdipLoadImageFromFile");
    ctx->GetWidth = (FnGdipGetImageWidth)GetProcAddress(ctx->hLib, "GdipGetImageWidth");
    ctx->GetHeight = (FnGdipGetImageHeight)GetProcAddress(ctx->hLib, "GdipGetImageHeight");
    ctx->SaveToStream = (FnGdipSaveImageToStream)GetProcAddress(ctx->hLib, "GdipSaveImageToStream");
    ctx->DisposeImage = (FnGdipDisposeImage)GetProcAddress(ctx->hLib, "GdipDisposeImage");

    if (!ctx->Startup || !ctx->Shutdown || !ctx->LoadImage ||
        !ctx->GetWidth || !ctx->GetHeight || !ctx->SaveToStream || !ctx->DisposeImage) {
        FreeLibrary(ctx->hLib);
        ctx->hLib = NULL;
        return false;
    }

    GpStartupInput input = { 1, NULL, FALSE, FALSE };
    if (ctx->Startup(&ctx->token, &input, NULL) != 0) {
        FreeLibrary(ctx->hLib);
        ctx->hLib = NULL;
        return false;
    }

    return true;
}

static void gdi_cleanup(GdiContext *ctx) {
    if (ctx && ctx->hLib) {
        if (ctx->Shutdown && ctx->token) {
            ctx->Shutdown(ctx->token);
        }
        FreeLibrary(ctx->hLib);
        memset(ctx, 0, sizeof(GdiContext));
    }
}

/* Fast dimension reader for JPEG 2000 (.jp2 / .j2k / .jpx) */
static bool read_jp2_dimensions(const wchar_t *path, uint32_t *out_w, uint32_t *out_h) {
    if (!path || !out_w || !out_h) return false;
    *out_w = 0;
    *out_h = 0;

    FILE *f = _wfopen(path, L"rb");
    if (!f) return false;

    uint8_t buf[2048];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    if (n < 16) return false;

    /* Check for JP2 box wrapper (signature box 00 00 00 0C 6A 50 20 20) */
    if (buf[4] == 'j' && buf[5] == 'P' && buf[6] == ' ' && buf[7] == ' ') {
        size_t pos = 0;
        while (pos + 8 <= n) {
            uint32_t blen = ((uint32_t)buf[pos] << 24) | ((uint32_t)buf[pos + 1] << 16) |
                            ((uint32_t)buf[pos + 2] << 8) | (uint32_t)buf[pos + 3];
            if (memcmp(&buf[pos + 4], "jp2h", 4) == 0) {
                size_t hpos = pos + 8;
                size_t hend = (blen > 0) ? (pos + blen) : n;
                if (hend > n) hend = n;
                while (hpos + 8 <= hend) {
                    uint32_t slen = ((uint32_t)buf[hpos] << 24) | ((uint32_t)buf[hpos + 1] << 16) |
                                    ((uint32_t)buf[hpos + 2] << 8) | (uint32_t)buf[hpos + 3];
                    if (memcmp(&buf[hpos + 4], "ihdr", 4) == 0 && hpos + 16 <= hend) {
                        *out_h = ((uint32_t)buf[hpos + 8] << 24) | ((uint32_t)buf[hpos + 9] << 16) |
                                 ((uint32_t)buf[hpos + 10] << 8) | (uint32_t)buf[hpos + 11];
                        *out_w = ((uint32_t)buf[hpos + 12] << 24) | ((uint32_t)buf[hpos + 13] << 16) |
                                 ((uint32_t)buf[hpos + 14] << 8) | (uint32_t)buf[hpos + 15];
                        return (*out_w > 0 && *out_h > 0);
                    }
                    if (slen == 0) break;
                    hpos += slen;
                }
            }
            if (blen == 0) break;
            pos += blen;
        }
    }

    /* Check for raw J2K codestream (starts with 0xFF4F, then 0xFF51 SIZ marker) */
    if (buf[0] == 0xFF && buf[1] == 0x4F) {
        size_t pos = 2;
        while (pos + 14 <= n) {
            if (buf[pos] == 0xFF && buf[pos + 1] == 0x51) {
                *out_w = ((uint32_t)buf[pos + 6] << 24) | ((uint32_t)buf[pos + 7] << 16) |
                         ((uint32_t)buf[pos + 8] << 8) | (uint32_t)buf[pos + 9];
                *out_h = ((uint32_t)buf[pos + 10] << 24) | ((uint32_t)buf[pos + 11] << 16) |
                         ((uint32_t)buf[pos + 12] << 8) | (uint32_t)buf[pos + 13];
                return (*out_w > 0 && *out_h > 0);
            }
            pos++;
        }
    }

    return false;
}

Img2PdfItemList* img2pdf_create_list(void) {
    Img2PdfItemList *list = (Img2PdfItemList *)malloc(sizeof(Img2PdfItemList));
    if (!list) return NULL;
    list->capacity = 32;
    list->count = 0;
    list->items = (Img2PdfItem *)malloc(list->capacity * sizeof(Img2PdfItem));
    if (!list->items) {
        free(list);
        return NULL;
    }
    return list;
}

void img2pdf_free_list(Img2PdfItemList *list) {
    if (!list) return;
    if (list->items) {
        free(list->items);
        list->items = NULL;
    }
    free(list);
}

void img2pdf_clear_list(Img2PdfItemList *list) {
    if (!list) return;
    list->count = 0;
}

bool img2pdf_add_item(Img2PdfItemList *list, const wchar_t *full_path, const wchar_t *file_name, uint64_t size) {
    if (!list || !full_path || !file_name) return false;

    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity * 2;
        Img2PdfItem *new_items = (Img2PdfItem *)realloc(list->items, new_cap * sizeof(Img2PdfItem));
        if (!new_items) return false;
        list->items = new_items;
        list->capacity = new_cap;
    }

    Img2PdfItem *item = &list->items[list->count];
    memset(item, 0, sizeof(Img2PdfItem));
    wcsncpy(item->full_path, full_path, (MAX_PATH * 2) - 1);
    wcsncpy(item->file_name, file_name, MAX_PATH - 1);
    item->file_size = size;
    item->page_num = (int)(list->count + 1);

    list->count++;
    return true;
}

bool img2pdf_is_supported_image(const wchar_t *filename) {
    if (!filename) return false;
    const wchar_t *ext = wcsrchr(filename, L'.');
    if (!ext) return false;

    if (_wcsicmp(ext, L".jpg") == 0 ||
        _wcsicmp(ext, L".jpeg") == 0 ||
        _wcsicmp(ext, L".png") == 0 ||
        _wcsicmp(ext, L".bmp") == 0 ||
        _wcsicmp(ext, L".gif") == 0 ||
        _wcsicmp(ext, L".tif") == 0 ||
        _wcsicmp(ext, L".tiff") == 0 ||
        _wcsicmp(ext, L".webp") == 0 ||
        _wcsicmp(ext, L".jp2") == 0 ||
        _wcsicmp(ext, L".j2k") == 0 ||
        _wcsicmp(ext, L".jpx") == 0 ||
        _wcsicmp(ext, L".jpm") == 0) {
        return true;
    }
    return false;
}

/* Natural sort comparison by filename (1, 2, 10, etc.) */
static int compare_img_items(const void *a, const void *b) {
    const Img2PdfItem *ia = (const Img2PdfItem *)a;
    const Img2PdfItem *ib = (const Img2PdfItem *)b;
    return StrCmpLogicalW(ia->file_name, ib->file_name);
}

static void scan_folder_internal(Img2PdfItemList *list, const wchar_t *folder_path, bool recursive) {
    wchar_t search_pattern[MAX_PATH * 2];
    swprintf(search_pattern, MAX_PATH * 2, L"%ls%ls*.*",
             folder_path,
             (folder_path[wcslen(folder_path) - 1] == L'\\') ? L"" : L"\\");

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search_pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) continue;

        wchar_t full_path[MAX_PATH * 2];
        swprintf(full_path, MAX_PATH * 2, L"%ls%ls%ls",
                 folder_path,
                 (folder_path[wcslen(folder_path) - 1] == L'\\') ? L"" : L"\\",
                 ffd.cFileName);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                scan_folder_internal(list, full_path, true);
            }
        } else {
            if (img2pdf_is_supported_image(ffd.cFileName)) {
                uint64_t size = ((uint64_t)ffd.nFileSizeHigh << 32) | ffd.nFileSizeLow;
                img2pdf_add_item(list, full_path, ffd.cFileName, size);
            }
        }
    } while (FindNextFileW(hFind, &ffd));
    FindClose(hFind);
}

bool img2pdf_scan_folder(Img2PdfItemList *list, const wchar_t *folder_path, bool recursive) {
    if (!list || !folder_path || wcslen(folder_path) == 0) return false;
    img2pdf_clear_list(list);

    scan_folder_internal(list, folder_path, recursive);

    /* If recursive was false but 0 images found in root, check if subfolders contain images */
    if (list->count == 0 && !recursive) {
        scan_folder_internal(list, folder_path, true);
    }

    if (list->count == 0) return true;

    /* Sort files in natural numerical and alphabetical sequence */
    if (list->count > 1) {
        qsort(list->items, list->count, sizeof(Img2PdfItem), compare_img_items);
    }

    /* Assign page numbering and get dimensions via GDI+ or JP2 header reader */
    GdiContext ctx;
    bool gdi_ok = gdi_init(&ctx);

    for (size_t i = 0; i < list->count; i++) {
        list->items[i].page_num = (int)(i + 1);

        const wchar_t *ext = wcsrchr(list->items[i].file_name, L'.');
        bool is_jp2 = ext && (_wcsicmp(ext, L".jp2") == 0 || _wcsicmp(ext, L".j2k") == 0 ||
                             _wcsicmp(ext, L".jpx") == 0 || _wcsicmp(ext, L".jpm") == 0);

        if (is_jp2) {
            read_jp2_dimensions(list->items[i].full_path, &list->items[i].width, &list->items[i].height);
        } else if (gdi_ok) {
            void *img = NULL;
            if (ctx.LoadImage(list->items[i].full_path, &img) == 0 && img) {
                UINT w = 0, h = 0;
                ctx.GetWidth(img, &w);
                ctx.GetHeight(img, &h);
                list->items[i].width = (uint32_t)w;
                list->items[i].height = (uint32_t)h;
                ctx.DisposeImage(img);
            }
        }

        if (list->items[i].width == 0 || list->items[i].height == 0) {
            read_jp2_dimensions(list->items[i].full_path, &list->items[i].width, &list->items[i].height);
        }
    }

    gdi_cleanup(&ctx);
    return true;
}

bool img2pdf_convert_to_pdf(
    const Img2PdfItemList *list,
    const wchar_t *output_pdf_path,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
) {
    if (!list || list->count == 0 || !output_pdf_path) return false;

    FILE *fout = _wfopen(output_pdf_path, L"wb");
    if (!fout) return false;

    GdiContext ctx;
    bool gdi_ok = gdi_init(&ctx);

    int num_pages = (int)list->count;
    /*
     * PDF Object Structure (PDF 1.5):
     * Object 1: Catalog
     * Object 2: Pages root
     * For each page i (0 <= i < num_pages):
     *   Page object:    3 + i * 3
     *   Content stream: 3 + i * 3 + 1
     *   Image XObject:  3 + i * 3 + 2
     */
    int total_objs = 2 + num_pages * 3;
    long *offsets = (long *)calloc(total_objs + 1, sizeof(long));
    if (!offsets) {
        fclose(fout);
        gdi_cleanup(&ctx);
        return false;
    }

    /* PDF 1.5 Header (supports both DCTDecode JPEG and JPXDecode JPEG 2000) */
    fprintf(fout, "%%PDF-1.5\r\n%%\xE2\xE3\xCF\xD3\r\n");

    /* Object 1: Catalog */
    offsets[1] = ftell(fout);
    fprintf(fout, "1 0 obj\r\n<<\r\n  /Type /Catalog\r\n  /Pages 2 0 R\r\n>>\r\nendobj\r\n");

    /* Object 2: Pages root */
    offsets[2] = ftell(fout);
    fprintf(fout, "2 0 obj\r\n<<\r\n  /Type /Pages\r\n  /Count %d\r\n  /Kids [", num_pages);
    for (int i = 0; i < num_pages; i++) {
        int page_obj_id = 3 + i * 3;
        fprintf(fout, " %d 0 R", page_obj_id);
    }
    fprintf(fout, " ]\r\n>>\r\nendobj\r\n");

    /* JPEG quality configuration (92%) for GDI+ re-encoding */
    ULONG quality = 92;
    GpEncoderParameters ep;
    ep.Count = 1;
    ep.Parameter[0].Guid = GUID_EncoderQuality;
    ep.Parameter[0].NumberOfValues = 1;
    ep.Parameter[0].Type = 4;
    ep.Parameter[0].Value = &quality;

    bool success = true;

    for (int i = 0; i < num_pages; i++) {
        if (cancel_flag && *cancel_flag) {
            success = false;
            break;
        }

        const Img2PdfItem *item = &list->items[i];
        int page_obj = 3 + i * 3;
        int content_obj = page_obj + 1;
        int image_obj = page_obj + 2;

        if (cb) {
            wchar_t status_msg[256];
            swprintf(status_msg, 256, L"Halaman %d / %d: %ls", i + 1, num_pages, item->file_name);
            cb(item->full_path, i + 1, num_pages, status_msg, user_data);
        }

        const wchar_t *ext = wcsrchr(item->file_name, L'.');
        bool is_jp2 = ext && (_wcsicmp(ext, L".jp2") == 0 || _wcsicmp(ext, L".j2k") == 0 ||
                             _wcsicmp(ext, L".jpx") == 0 || _wcsicmp(ext, L".jpm") == 0);

        unsigned char *img_buf = NULL;
        ULONG img_size = 0;
        UINT w = item->width;
        UINT h = item->height;

        if (is_jp2) {
            /* Directly embed JPEG 2000 codestream with /Filter /JPXDecode (lossless, instant) */
            FILE *fin = _wfopen(item->full_path, L"rb");
            if (fin) {
                fseek(fin, 0, SEEK_END);
                long fsz = ftell(fin);
                fseek(fin, 0, SEEK_SET);
                if (fsz > 0) {
                    img_buf = (unsigned char *)malloc(fsz);
                    if (img_buf) {
                        size_t r = fread(img_buf, 1, fsz, fin);
                        if (r == (size_t)fsz) {
                            img_size = (ULONG)fsz;
                        } else {
                            free(img_buf);
                            img_buf = NULL;
                        }
                    }
                }
                fclose(fin);
            }
            if (w == 0 || h == 0) {
                read_jp2_dimensions(item->full_path, &w, &h);
            }
        } else if (gdi_ok) {
            /* Encode other image formats (PNG, BMP, GIF, TIFF, WebP, JPG) to JPEG via GDI+ */
            void *img = NULL;
            if (ctx.LoadImage(item->full_path, &img) == 0 && img) {
                ctx.GetWidth(img, &w);
                ctx.GetHeight(img, &h);

                IStream *pStream = NULL;
                if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pStream))) {
                    if (ctx.SaveToStream(img, pStream, &CLSID_JpegEncoder, &ep) == 0) {
                        STATSTG stat;
                        pStream->lpVtbl->Stat(pStream, &stat, STATFLAG_NONAME);
                        img_size = (ULONG)stat.cbSize.QuadPart;
                        if (img_size > 0) {
                            img_buf = (unsigned char *)malloc(img_size);
                            if (img_buf) {
                                LARGE_INTEGER dlibZero = {0};
                                pStream->lpVtbl->Seek(pStream, dlibZero, STREAM_SEEK_SET, NULL);
                                ULONG readBytes = 0;
                                pStream->lpVtbl->Read(pStream, img_buf, img_size, &readBytes);
                            }
                        }
                    }
                    pStream->lpVtbl->Release(pStream);
                }
                ctx.DisposeImage(img);
            }
        }

        if (!img_buf || img_size == 0) {
            if (img_buf) free(img_buf);
            success = false;
            break;
        }

        if (w == 0) w = 800;
        if (h == 0) h = 600;

        /* Page MediaBox in PDF points (72 DPI) */
        uint32_t pw = (uint32_t)((double)w * 72.0 / 96.0);
        uint32_t ph = (uint32_t)((double)h * 72.0 / 96.0);
        if (pw == 0) pw = w;
        if (ph == 0) ph = h;

        /* Page Object */
        offsets[page_obj] = ftell(fout);
        fprintf(fout, "%d 0 obj\r\n<<\r\n  /Type /Page\r\n  /Parent 2 0 R\r\n  /MediaBox [0 0 %u %u]\r\n"
                      "  /Contents %d 0 R\r\n  /Resources <<\r\n    /ProcSet [/PDF /ImageC]\r\n"
                      "    /XObject << /Im%d %d 0 R >>\r\n  >>\r\n>>\r\nendobj\r\n",
                page_obj, pw, ph, content_obj, i, image_obj);

        /* Content Object: map image into page coordinates */
        char content_stream[128];
        int c_len = sprintf(content_stream, "q\r\n%u 0 0 %u 0 0 cm\r\n/Im%d Do\r\nQ\r\n", pw, ph, i);
        offsets[content_obj] = ftell(fout);
        fprintf(fout, "%d 0 obj\r\n<<\r\n  /Length %d\r\n>>\r\nstream\r\n%sendstream\r\nendobj\r\n",
                content_obj, c_len, content_stream);

        /* Image XObject */
        offsets[image_obj] = ftell(fout);
        if (is_jp2) {
            fprintf(fout, "%d 0 obj\r\n<<\r\n  /Type /XObject\r\n  /Subtype /Image\r\n  /Width %u\r\n  /Height %u\r\n"
                          "  /Filter /JPXDecode\r\n  /Length %lu\r\n>>\r\nstream\r\n",
                    image_obj, w, h, img_size);
        } else {
            fprintf(fout, "%d 0 obj\r\n<<\r\n  /Type /XObject\r\n  /Subtype /Image\r\n  /Width %u\r\n  /Height %u\r\n"
                          "  /ColorSpace /DeviceRGB\r\n  /BitsPerComponent 8\r\n  /Filter /DCTDecode\r\n  /Length %lu\r\n>>\r\nstream\r\n",
                    image_obj, w, h, img_size);
        }
        fwrite(img_buf, 1, img_size, fout);
        fprintf(fout, "\r\nendstream\r\nendobj\r\n");

        free(img_buf);
    }

    if (success) {
        /* Cross-Reference Table */
        long xref_offset = ftell(fout);
        fprintf(fout, "xref\r\n0 %d\r\n", total_objs + 1);
        fprintf(fout, "0000000000 65535 f \r\n");
        for (int i = 1; i <= total_objs; i++) {
            fprintf(fout, "%010ld 00000 n \r\n", offsets[i]);
        }

        /* Trailer Dictionary */
        fprintf(fout, "trailer\r\n<<\r\n  /Size %d\r\n  /Root 1 0 R\r\n>>\r\nstartxref\r\n%ld\r\n%%%%EOF\r\n",
                total_objs + 1, xref_offset);
    }

    free(offsets);
    fclose(fout);
    gdi_cleanup(&ctx);

    if (!success) {
        _wremove(output_pdf_path);
        return false;
    }

    return true;
}
