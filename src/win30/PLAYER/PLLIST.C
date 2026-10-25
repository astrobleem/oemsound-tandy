#include <string.h>
#include "PLLIST.H"

static unsigned short fold_ascii(unsigned char c)
{
    if (c >= (unsigned char)'A' && c <= (unsigned char)'Z')
        return (unsigned short)(c + (unsigned char)('a' - 'A'));
    return (unsigned short)c;
}

static void ensure_visible(PLLModel *model, unsigned short visibleRows)
{
    if (model->count == 0) {
        model->selected = PLL_NONE;
        model->top = 0;
        return;
    }
    if (visibleRows == 0) visibleRows = 1;
    if (model->selected >= model->count) model->selected = 0;
    if (model->selected < model->top)
        model->top = model->selected;
    else if ((unsigned short)(model->selected - model->top) >= visibleRows)
        model->top = (unsigned short)(model->selected - visibleRows + 1U);
    if (model->top >= model->count) model->top = (unsigned short)(model->count - 1U);
}

static int same_path(const PLLModel *model, unsigned short index,
                     const char *path, unsigned short length)
{
    unsigned short i;
    const char *stored;
    if ((unsigned short)strlen(model->path[index]) != length) return 0;
    stored = model->path[index];
    for (i = 0; i < length; ++i) {
        if (fold_ascii((unsigned char)stored[i]) !=
            fold_ascii((unsigned char)path[i])) return 0;
    }
    return 1;
}

void PLLInit(PLLModel *model)
{
    if (model == 0) return;
    memset(model, 0, sizeof(*model));
    model->selected = PLL_NONE;
}

PLLAddStatus PLLAdd(PLLModel *model, const char *path)
{
    unsigned short length;
    unsigned short i;

    if (model == 0 || path == 0 || path[0] == '\0') return PLL_ADD_INVALID;
    for (length = 0; length <= PLL_PATH_MAX && path[length] != '\0'; ++length) {
        /* The loop checks the terminator at index 127 before accepting. */
    }
    if (length == 0 || length > PLL_PATH_MAX) return PLL_ADD_INVALID;
    for (i = 0; i < model->count; ++i)
        if (same_path(model, i, path, length)) return PLL_ADD_DUPLICATE;
    if (model->count >= PLL_CAPACITY) return PLL_ADD_FULL;
    memcpy(model->path[model->count], path, length);
    model->path[model->count][length] = '\0';
    ++model->count;
    if (model->selected == PLL_NONE) {
        model->selected = 0;
        model->top = 0;
    }
    return PLL_ADD_ADDED;
}

PLLResult PLLSelect(PLLModel *model, unsigned short index,
                    unsigned short visibleRows)
{
    if (model == 0 || index >= model->count || visibleRows == 0)
        return PLL_INVALID;
    model->selected = index;
    ensure_visible(model, visibleRows);
    return PLL_OK;
}

PLLResult PLLRemove(PLLModel *model, unsigned short index,
                    unsigned short visibleRows)
{
    unsigned short i;

    if (model == 0 || index >= model->count || visibleRows == 0)
        return PLL_INVALID;
    for (i = index; i + 1U < model->count; ++i) {
        memcpy(model->path[i], model->path[i + 1U], PLL_PATH_MAX + 1);
    }
    --model->count;
    memset(model->path[model->count], 0, PLL_PATH_MAX + 1);
    if (model->count == 0) {
        model->selected = PLL_NONE;
        model->top = 0;
    } else if (model->selected == index) {
        if (index >= model->count) model->selected = (unsigned short)(model->count - 1U);
        else model->selected = index;
    } else if (model->selected > index) {
        --model->selected;
    }
    ensure_visible(model, visibleRows);
    return PLL_OK;
}

void PLLClear(PLLModel *model)
{
    if (model == 0) return;
    memset(model, 0, sizeof(*model));
    model->selected = PLL_NONE;
}

PLLResult PLLScroll(PLLModel *model, unsigned short command,
                    unsigned short visibleRows)
{
    unsigned short step;
    if (model == 0 || model->count == 0 || visibleRows == 0)
        return PLL_INVALID;
    if (model->selected >= model->count) model->selected = 0;
    step = visibleRows;
    switch (command) {
    case PLL_SCROLL_UP:
        if (model->selected != 0) --model->selected;
        break;
    case PLL_SCROLL_DOWN:
        if (model->selected + 1U < model->count) ++model->selected;
        break;
    case PLL_SCROLL_HOME:
        model->selected = 0;
        break;
    case PLL_SCROLL_END:
        model->selected = (unsigned short)(model->count - 1U);
        break;
    case PLL_SCROLL_PAGE_UP:
        if (model->selected > step) model->selected -= step;
        else model->selected = 0;
        break;
    case PLL_SCROLL_PAGE_DOWN:
        if (step > (unsigned short)(model->count - 1U - model->selected))
            model->selected = (unsigned short)(model->count - 1U);
        else model->selected += step;
        break;
    default:
        return PLL_INVALID;
    }
    ensure_visible(model, visibleRows);
    return PLL_OK;
}

const char *PLLGet(const PLLModel *model, unsigned short index)
{
    if (model == 0 || index >= model->count) return 0;
    return model->path[index];
}

