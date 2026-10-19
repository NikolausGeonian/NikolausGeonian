#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "pressure_calc.h"

enum { ID_P1=101, ID_T1, ID_P2, ID_T2, ID_TYPE=110, ID_UNIT,
       ID_CALCULATE=120, ID_CLEAR, ID_RESULT1=130, ID_RESULT2, ID_RATE };
static HWND inputs[4], outputs[3], type_box, unit_box, status_label;
static HWND pressure_labels[2], result_labels[2], atmosphere_label, rate_label;
static HWND calculate_button, clear_button;
static HFONT font_regular, font_title, font_result;
static HBRUSH background_brush, white_brush, result_brush;
static int dpi=96, internal_change=0, status_is_error=0;
static const int content_height=560;
static const wchar_t *units[]={L"MPa", L"kPa", L"bar", L"Pa"};
static const double atmospheres[]={0.101325, 101.325, 1.01325, 101325.0};
static const COLORREF ink=RGB(28,44,63), muted=RGB(85,103,122);
static const COLORREF background=RGB(246,248,251), accent=RGB(0,105,112);

static int px(int value) { return MulDiv(value,dpi,96); }
static void set_status(const wchar_t *text, int error) {
    status_is_error=error;
    SetWindowTextW(status_label,text);
    InvalidateRect(status_label,NULL,TRUE);
}
static void reset_outputs(void) {
    for (int i=0;i<3;i++) SetWindowTextW(outputs[i],L"0");
}
static void pending(void) {
    reset_outputs();
    set_status(L"数据已修改，请点击“计算数据”。",0);
}
static void clear_data(void) {
    internal_change=1;
    for(int i=0;i<4;i++) SetWindowTextW(inputs[i],L"0");
    reset_outputs();
    internal_change=0;
    set_status(L"所有文本框已归零。请输入两组压力和温度。",0);
    SetFocus(inputs[0]);
    SendMessageW(inputs[0],EM_SETSEL,0,-1);
}

/* Only decimal numbers are accepted: no NaN, infinity, hex, or trailing text. */
static int read_number(HWND edit, double *value) {
    wchar_t text[128], *end;
    GetWindowTextW(edit,text,128);
    const wchar_t *s=text;
    while(iswspace(*s)) s++;
    if(*s==L'+' || *s==L'-') s++;
    int digits=0;
    while(*s>=L'0' && *s<=L'9') { s++; digits++; }
    if(*s==L'.') {
        s++;
        while(*s>=L'0' && *s<=L'9') { s++; digits++; }
    }
    if(!digits) return 0;
    if(*s==L'e' || *s==L'E') {
        s++;
        if(*s==L'+' || *s==L'-') s++;
        int exponent_digits=0;
        while(*s>=L'0' && *s<=L'9') { s++; exponent_digits++; }
        if(!exponent_digits) return 0;
    }
    while(iswspace(*s)) s++;
    if(*s) return 0;
    errno=0;
    *value=wcstod(text,&end);
    return errno!=ERANGE && isfinite(*value);
}

static void update_labels(void) {
    int unit=(int)SendMessageW(unit_box,CB_GETCURSEL,0,0);
    int gauge=SendMessageW(type_box,CB_GETCURSEL,0,0)==0;
    wchar_t text[160];
    for(int i=0;i<2;i++) {
        swprintf(text,160,L"%ls（%ls）",gauge?L"表压":L"绝对压力",units[unit]);
        SetWindowTextW(pressure_labels[i],text);
        swprintf(text,160,L"第 %d 组 · 25 ℃ %ls（%ls）",i+1,gauge?L"表压":L"绝压",units[unit]);
        SetWindowTextW(result_labels[i],text);
    }
    swprintf(text,160,L"%ls下降率（%%）",gauge?L"表压":L"绝压");
    SetWindowTextW(rate_label,text);
    if(gauge) swprintf(text,160,L"采用标准大气压：%.6g %ls",atmospheres[unit],units[unit]);
    else wcscpy(text,L"绝对压力直接按绝对温度换算");
    SetWindowTextW(atmosphere_label,text);
}

static void calculate_data(void) {
    const wchar_t *names[]={L"第 1 组压力",L"第 1 组温度",L"第 2 组压力",L"第 2 组温度"};
    double values[4];
    wchar_t text[240];
    reset_outputs();
    for(int i=0;i<4;i++) {
        if(!read_number(inputs[i],&values[i])) {
            swprintf(text,240,L"%ls：请输入有效数字，小数点使用英文句点。",names[i]);
            set_status(text,1);
            SetFocus(inputs[i]);
            SendMessageW(inputs[i],EM_SETSEL,0,-1);
            return;
        }
    }
    int unit=(int)SendMessageW(unit_box,CB_GETCURSEL,0,0);
    PressureKind kind=(PressureKind)SendMessageW(type_box,CB_GETCURSEL,0,0);
    PressureResult result;
    int error=pressure_calculate(values[0],values[1],values[2],values[3],kind,atmospheres[unit],&result);
    if(error) { set_status(pressure_error_message(error),1); return; }
    swprintf(text,240,L"%.10g",result.pressure25_first);
    SetWindowTextW(outputs[0],text);
    swprintf(text,240,L"%.10g",result.pressure25_second);
    SetWindowTextW(outputs[1],text);
    if(result.decrease_defined) {
        swprintf(text,240,fabs(result.decrease_percent)>=1e6?L"%.6g":L"%.6f",result.decrease_percent);
        SetWindowTextW(outputs[2],text);
        set_status(result.decrease_percent<0
            ? L"计算完成。下降率为负值，表示第 2 组换算压力升高。"
            : L"计算完成。下降率为第 2 组相对第 1 组的换算压力下降比例。",0);
    } else {
        SetWindowTextW(outputs[2],L"无法计算");
        set_status(L"压力换算完成；第 1 组换算压力 ≤ 0，下降率无定义。",1);
    }
}

static HWND control(HWND parent,const wchar_t *cls,const wchar_t *text,
                    DWORD style,int x,int y,int w,int h,int id) {
    HWND child=CreateWindowExW(wcscmp(cls,L"EDIT")==0?WS_EX_CLIENTEDGE:0,cls,text,
        WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(w),px(h),parent,
        (HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
    SendMessageW(child,WM_SETFONT,(WPARAM)font_regular,TRUE);
    return child;
}
static HWND label(HWND parent,const wchar_t *text,int x,int y,int w,int h) {
    return control(parent,L"STATIC",text,SS_LEFT,x,y,w,h,0);
}
static void make_controls(HWND hwnd) {
    HWND title=label(hwnd,L"压力温度换算",32,18,550,36);
    SendMessageW(title,WM_SETFONT,(WPARAM)font_title,TRUE);
    label(hwnd,L"输入两组读数，统一换算至 25 ℃，比较压力变化。",34,58,660,26);
    label(hwnd,L"压力类型",34,98,78,24);
    type_box=control(hwnd,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,114,93,152,110,ID_TYPE);
    SendMessageW(type_box,CB_ADDSTRING,0,(LPARAM)L"表压");
    SendMessageW(type_box,CB_ADDSTRING,0,(LPARAM)L"绝对压力");
    SendMessageW(type_box,CB_SETCURSEL,0,0);
    label(hwnd,L"单位",290,98,46,24);
    unit_box=control(hwnd,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_TABSTOP,338,93,96,160,ID_UNIT);
    for(int i=0;i<4;i++) SendMessageW(unit_box,CB_ADDSTRING,0,(LPARAM)units[i]);
    SendMessageW(unit_box,CB_SETCURSEL,0,0);
    atmosphere_label=label(hwnd,L"",452,98,290,26);
    for(int i=0;i<2;i++) {
        int x=34+i*366;
        label(hwnd,i==0?L"第 1 组（初始）":L"第 2 组（后续）",x,141,300,27);
        pressure_labels[i]=label(hwnd,L"",x,182,140,26);
        inputs[i*2]=control(hwnd,L"EDIT",L"0",WS_TABSTOP|ES_AUTOHSCROLL,x+144,177,174,33,ID_P1+i*2);
        label(hwnd,L"温度（℃）",x,226,140,26);
        inputs[i*2+1]=control(hwnd,L"EDIT",L"0",WS_TABSTOP|ES_AUTOHSCROLL,x+144,221,174,33,ID_T1+i*2);
    }
    for(int i=0;i<4;i++) SendMessageW(inputs[i],EM_SETLIMITTEXT,80,0);
    calculate_button=control(hwnd,L"BUTTON",L"计算数据",WS_TABSTOP|BS_OWNERDRAW,34,276,180,44,ID_CALCULATE);
    clear_button=control(hwnd,L"BUTTON",L"清空数据",WS_TABSTOP|BS_OWNERDRAW,230,276,150,44,ID_CLEAR);
    label(hwnd,L"换算结果",34,341,200,26);
    for(int i=0;i<3;i++) {
        int x=34+i*244;
        HWND result_label=label(hwnd,L"",x,378,230,26);
        if(i<2) result_labels[i]=result_label; else rate_label=result_label;
        outputs[i]=control(hwnd,L"EDIT",L"0",ES_READONLY|ES_AUTOHSCROLL|WS_TABSTOP,x,410,222,38,ID_RESULT1+i);
        SendMessageW(outputs[i],WM_SETFONT,(WPARAM)font_result,TRUE);
    }
    status_label=label(hwnd,L"请输入两组数据，然后点击“计算数据”。",34,462,700,42);
    label(hwnd,L"下降率 =（第 1 组换算压力 − 第 2 组换算压力）÷ 第 1 组换算压力 × 100%",34,504,700,25);
    label(hwnd,L"适用：定容理想气体。温度须大于 −273.15 ℃；表压换算包含标准大气压修正。",34,532,700,25);
    update_labels();
}

static void draw_button(DRAWITEMSTRUCT *item) {
    int primary=item->CtlID==ID_CALCULATE;
    COLORREF fill=primary?accent:RGB(255,255,255);
    if(item->itemState&ODS_SELECTED) fill=primary?RGB(0,79,86):RGB(229,236,242);
    HBRUSH brush=CreateSolidBrush(fill);
    HPEN pen=CreatePen(PS_SOLID,px(1),primary?fill:RGB(194,207,218));
    HGDIOBJ old_brush=SelectObject(item->hDC,brush),old_pen=SelectObject(item->hDC,pen);
    RECT r=item->rcItem;
    RoundRect(item->hDC,r.left,r.top,r.right,r.bottom,px(8),px(8));
    SelectObject(item->hDC,font_regular);
    SetBkMode(item->hDC,TRANSPARENT);
    SetTextColor(item->hDC,primary?RGB(255,255,255):ink);
    wchar_t text[32]; GetWindowTextW(item->hwndItem,text,32);
    DrawTextW(item->hDC,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(item->itemState&ODS_FOCUS) { InflateRect(&r,-px(4),-px(4)); DrawFocusRect(item->hDC,&r); }
    SelectObject(item->hDC,old_brush); SelectObject(item->hDC,old_pen);
    DeleteObject(brush); DeleteObject(pen);
}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_CREATE: make_controls(hwnd); return 0;
    case WM_SIZE: {
        SCROLLINFO info={0}; info.cbSize=sizeof(info); info.fMask=SIF_POS;
        GetScrollInfo(hwnd,SB_VERT,&info); int old=info.nPos;
        info.fMask=SIF_RANGE|SIF_PAGE; info.nMin=0; info.nMax=px(content_height)-1;
        info.nPage=HIWORD(lp); SetScrollInfo(hwnd,SB_VERT,&info,TRUE);
        info.fMask=SIF_POS; GetScrollInfo(hwnd,SB_VERT,&info);
        if(old!=info.nPos) ScrollWindowEx(hwnd,0,old-info.nPos,NULL,NULL,NULL,NULL,SW_SCROLLCHILDREN|SW_INVALIDATE|SW_ERASE);
        return 0;
    }
    case WM_MOUSEWHEEL:
        SendMessageW(hwnd,WM_VSCROLL,GET_WHEEL_DELTA_WPARAM(wp)>0?SB_LINEUP:SB_LINEDOWN,0);
        return 0;
    case WM_VSCROLL: {
        SCROLLINFO info={0}; info.cbSize=sizeof(info); info.fMask=SIF_ALL;
        GetScrollInfo(hwnd,SB_VERT,&info); int old=info.nPos;
        switch(LOWORD(wp)) {
        case SB_LINEUP: info.nPos-=px(30); break;
        case SB_LINEDOWN: info.nPos+=px(30); break;
        case SB_PAGEUP: info.nPos-=(int)info.nPage; break;
        case SB_PAGEDOWN: info.nPos+=(int)info.nPage; break;
        case SB_THUMBTRACK: info.nPos=info.nTrackPos; break;
        case SB_TOP: info.nPos=info.nMin; break;
        case SB_BOTTOM: info.nPos=info.nMax; break;
        }
        info.fMask=SIF_POS; SetScrollInfo(hwnd,SB_VERT,&info,TRUE); GetScrollInfo(hwnd,SB_VERT,&info);
        ScrollWindowEx(hwnd,0,old-info.nPos,NULL,NULL,NULL,NULL,SW_SCROLLCHILDREN|SW_INVALIDATE|SW_ERASE);
        return 0;
    }
    case WM_COMMAND:
        if(LOWORD(wp)==ID_CALCULATE && HIWORD(wp)==BN_CLICKED) { calculate_data(); return 0; }
        if(LOWORD(wp)==ID_CLEAR && HIWORD(wp)==BN_CLICKED) { clear_data(); return 0; }
        if(LOWORD(wp)>=ID_P1 && LOWORD(wp)<=ID_T2 && HIWORD(wp)==EN_CHANGE && !internal_change && status_label) {
            pending(); return 0;
        }
        if((LOWORD(wp)==ID_TYPE || LOWORD(wp)==ID_UNIT) && HIWORD(wp)==CBN_SELCHANGE) {
            update_labels(); pending(); return 0;
        }
        break;
    case WM_DRAWITEM: draw_button((DRAWITEMSTRUCT*)lp); return TRUE;
    case WM_CTLCOLOREDIT:
        SetTextColor((HDC)wp,ink); SetBkColor((HDC)wp,RGB(255,255,255)); return (LRESULT)white_brush;
    case WM_CTLCOLORSTATIC:
        for(int i=0;i<3;i++) if((HWND)lp==outputs[i]) {
            SetTextColor((HDC)wp,accent); SetBkColor((HDC)wp,RGB(232,242,244)); return (LRESULT)result_brush;
        }
        SetTextColor((HDC)wp,(HWND)lp==status_label && status_is_error?RGB(172,51,38):muted);
        SetBkColor((HDC)wp,background); return (LRESULT)background_brush;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

static HWND create_app(HINSTANCE instance) {
    SetProcessDPIAware();
    HDC dc=GetDC(NULL); dpi=GetDeviceCaps(dc,LOGPIXELSX); ReleaseDC(NULL,dc);
    font_regular=CreateFontW(-px(16),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
    font_title=CreateFontW(-px(28),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
    font_result=CreateFontW(-px(21),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
    background_brush=CreateSolidBrush(background);
    white_brush=CreateSolidBrush(RGB(255,255,255));
    result_brush=CreateSolidBrush(RGB(232,242,244));
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_STANDARD_CLASSES}; InitCommonControlsEx(&controls);
    WNDCLASSW wc={0};
    wc.lpfnWndProc=window_proc; wc.hInstance=instance; wc.lpszClassName=L"Pressure25Window";
    wc.hCursor=LoadCursorW(NULL,IDC_ARROW); wc.hIcon=LoadIconW(NULL,IDI_APPLICATION);
    wc.hbrBackground=background_brush;
    if(!RegisterClassW(&wc)) return NULL;
    DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VSCROLL;
    RECT rect={0,0,px(766),px(content_height)}; AdjustWindowRectEx(&rect,style,FALSE,WS_EX_CONTROLPARENT);
    RECT work; SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    int height=rect.bottom-rect.top;
    if(height>work.bottom-work.top-px(16)) height=work.bottom-work.top-px(16);
    return CreateWindowExW(WS_EX_CONTROLPARENT,wc.lpszClassName,L"压力温度换算 · 25 ℃",style,
        work.left+px(8),work.top+px(8),rect.right-rect.left,height,NULL,NULL,instance,NULL);
}
static void free_resources(void) {
    DeleteObject(font_regular); DeleteObject(font_title); DeleteObject(font_result);
    DeleteObject(background_brush); DeleteObject(white_brush); DeleteObject(result_brush);
}

#ifdef GUI_TEST
#include "test_gui.inc"
#else
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,PWSTR command,int show) {
    (void)previous; (void)command;
    HWND hwnd=create_app(instance);
    if(!hwnd) { MessageBoxW(NULL,L"无法创建程序窗口。",L"启动失败",MB_OK|MB_ICONERROR); return 1; }
    ShowWindow(hwnd,show); UpdateWindow(hwnd); SetFocus(inputs[0]);
    MSG msg;
    int result;
    while((result=GetMessageW(&msg,NULL,0,0))>0) {
        if(!IsDialogMessageW(hwnd,&msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    free_resources();
    return result==-1?1:(int)msg.wParam;
}
#endif
