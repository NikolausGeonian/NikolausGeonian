/* Compiled only for the test executable, never included in Pressure25.exe. */
#include <assert.h>
static void expect_text(HWND field,const wchar_t *expected) {
    wchar_t text[256]; GetWindowTextW(field,text,256);
    if(wcscmp(text,expected)!=0) {
        fwprintf(stderr,L"Expected [%ls], got [%ls]\n",expected,text); assert(0);
    }
}
static void expect_number(HWND field,double expected,double tolerance) {
    double actual=0;
    assert(read_number(field,&actual));
    assert(fabs(actual-expected)<=tolerance);
}
static void set_inputs(const wchar_t *p1,const wchar_t *t1,const wchar_t *p2,const wchar_t *t2) {
    SetWindowTextW(inputs[0],p1); SetWindowTextW(inputs[1],t1);
    SetWindowTextW(inputs[2],p2); SetWindowTextW(inputs[3],t2);
}
static void click(HWND button) { SendMessageW(button,BM_CLICK,0,0); }
int wmain(void) {
    HWND hwnd=create_app(GetModuleHandleW(NULL)); assert(hwnd);
    ShowWindow(hwnd,SW_SHOW); UpdateWindow(hwnd);
    assert(SendMessageW(type_box,CB_GETCURSEL,0,0)==0);
    assert(SendMessageW(unit_box,CB_GETCURSEL,0,0)==0);
    for(int i=0;i<4;i++) expect_text(inputs[i],L"0");
    for(int i=0;i<3;i++) expect_text(outputs[i],L"0");
    puts("PASS: default gauge MPa; all seven text boxes start at zero");

    set_inputs(L"0.5",L"25",L"0.5",L"35");
    for(int i=0;i<3;i++) expect_text(outputs[i],L"0");
    click(calculate_button);
    expect_number(outputs[0],0.5,1e-12);
    expect_number(outputs[1],0.4804859646,1e-10);
    expect_number(outputs[2],3.902807,1e-6);
    puts("PASS: editing does not calculate; Calculate produces correct gauge results");

    SetWindowTextW(inputs[2],L"0.6");
    for(int i=0;i<3;i++) expect_text(outputs[i],L"0");
    set_inputs(L"0.5",L"25",L"0.6",L"25"); click(calculate_button);
    expect_number(outputs[2],-20.0,1e-6);
    puts("PASS: edited input invalidates prior results; pressure rise is negative decrease");

    click(clear_button);
    for(int i=0;i<4;i++) expect_text(inputs[i],L"0");
    for(int i=0;i<3;i++) expect_text(outputs[i],L"0");
    click(clear_button);
    for(int i=0;i<3;i++) expect_text(outputs[i],L"0");
    puts("PASS: Clear resets all seven boxes to literal zero and is repeatable");

    set_inputs(L"0",L"25",L"0",L"35"); click(calculate_button);
    expect_number(outputs[0],0,0); expect_text(outputs[2],L"无法计算");
    assert(status_is_error);
    puts("PASS: zero baseline retains converted pressures and rejects undefined ratio");

    const wchar_t *invalid[]={L"",L" ",L"abc",L"NaN",L"inf",L"1.2x",L"0x10",L"1e999",L"1,5",L"1e+"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++) {
        set_inputs(invalid[i],L"25",L"0.4",L"25"); click(calculate_button);
        assert(status_is_error);
        for(int j=0;j<3;j++) expect_text(outputs[j],L"0");
    }
    set_inputs(L"0.5",L"-273.15",L"0.4",L"25"); click(calculate_button); assert(status_is_error);
    set_inputs(L"-0.101325",L"25",L"0.4",L"25"); click(calculate_button); assert(status_is_error);
    set_inputs(L"  +5e-1  ",L"25",L".4",L"25"); click(calculate_button);
    assert(!status_is_error); expect_number(outputs[2],20,1e-6);
    puts("PASS: invalid numbers, impossible temperature/pressure rejected; valid decimals accepted");

    SendMessageW(unit_box,CB_SETCURSEL,1,0);
    SendMessageW(hwnd,WM_COMMAND,MAKEWPARAM(ID_UNIT,CBN_SELCHANGE),(LPARAM)unit_box);
    for(int i=0;i<3;i++) expect_text(outputs[i],L"0");
    set_inputs(L"500",L"25",L"500",L"35"); click(calculate_button);
    expect_number(outputs[1],480.4859646,1e-7); expect_number(outputs[2],3.902807,1e-6);
    SendMessageW(type_box,CB_SETCURSEL,1,0);
    SendMessageW(hwnd,WM_COMMAND,MAKEWPARAM(ID_TYPE,CBN_SELCHANGE),(LPARAM)type_box);
    set_inputs(L"600",L"25",L"1200",L"323.15"); click(calculate_button);
    expect_number(outputs[0],600,1e-8); expect_number(outputs[1],600,1e-8);
    expect_number(outputs[2],0,1e-6);
    puts("PASS: units and absolute-pressure option calculate correctly");

    set_inputs(L"1",L"25",L"1e305",L"25"); click(calculate_button);
    assert(!status_is_error);
    expect_number(outputs[2],-1e307,1e301);
    assert(GetWindowTextLengthW(outputs[2])<40);
    puts("PASS: extreme finite percentage uses bounded scientific notation");

    RECT before,after;
    GetWindowRect(clear_button,&before);
    SetWindowPos(hwnd,NULL,0,0,px(790),px(300),SWP_NOMOVE|SWP_NOZORDER);
    SendMessageW(hwnd,WM_VSCROLL,SB_BOTTOM,0);
    GetWindowRect(clear_button,&after);
    assert(after.top<before.top);
    SendMessageW(hwnd,WM_VSCROLL,SB_TOP,0);
    GetWindowRect(clear_button,&after);
    assert(after.top==before.top);
    puts("PASS: constrained-height window scrolls to expose lower content");

    /* Count actual controls: only Calculate and Clear are offered as buttons. */
    int edits=0,buttons=0; wchar_t cls[64];
    for(HWND child=GetWindow(hwnd,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) {
        GetClassNameW(child,cls,64);
        if(_wcsicmp(cls,L"EDIT")==0) edits++;
        if(_wcsicmp(cls,L"BUTTON")==0) buttons++;
    }
    assert(edits==7 && buttons==2);
    for(int i=0;i<3;i++) assert(GetWindowLongPtrW(outputs[i],GWL_STYLE)&ES_READONLY);
    puts("PASS: seven text boxes, exactly two buttons, results read-only, no sample-data feature");
    DestroyWindow(hwnd); free_resources();
    puts("All 10 GUI test groups passed.");
    return 0;
}
