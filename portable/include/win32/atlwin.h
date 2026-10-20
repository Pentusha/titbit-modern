#ifndef TITBIT_PORTABLE_ATLWIN_H
#define TITBIT_PORTABLE_ATLWIN_H
#include <atlbase.h>
/* Small, project-owned dialog adapter. The recovered 28-byte dialog base is
 * retained, but dispatch uses Win32 dialog user data rather than allocating
 * executable x86 thunks. All UI operations are real Win32 API calls. */
namespace ATL {
class CWindow {
public:
    HWND m_hWnd;
    CWindow(HWND window=0):m_hWnd(window) { }
    CWindow& operator=(HWND window) {m_hWnd=window;return *this;}
    operator HWND() const {return m_hWnd;}
    HWND GetParent() const {return ::GetParent(m_hWnd);}
    HWND GetDlgItem(int id) const {return ::GetDlgItem(m_hWnd,id);}
    LRESULT SendMessageA(UINT message,WPARAM wparam=0,LPARAM lparam=0) const {return ::SendMessageA(m_hWnd,message,wparam,lparam);}
    BOOL EndDialog(int result) {return ::EndDialog(m_hWnd,result);}
    BOOL CenterWindow(HWND parent=0) {
        RECT area,window;
        if (!::GetWindowRect(m_hWnd,&window)) return FALSE;
        if (!parent) parent=::GetParent(m_hWnd);
        if (!parent || !::GetWindowRect(parent,&area)) {
            if (!::SystemParametersInfoA(SPI_GETWORKAREA,0,&area,0)) return FALSE;
        }
        return ::SetWindowPos(m_hWnd,0,
            area.left+((area.right-area.left)-(window.right-window.left))/2,
            area.top+((area.bottom-area.top)-(window.bottom-window.top))/2,
            0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    }
};
class ATL_NO_VTABLE CMessageMap {
public:
    virtual BOOL ProcessWindowMessage(HWND,UINT,WPARAM,LPARAM,LRESULT&,DWORD=0)=0;
};
struct CWndProcThunk {
    void *context;
    DWORD ownerThread;
    void *next;
    WNDPROC callback;
    CWndProcThunk():context(0),ownerThread(0),next(0),callback(0) { }
};
template<class TBase=CWindow> class ATL_NO_VTABLE CDialogImplBaseT : public CMessageMap,public TBase {
public:
    CWndProcThunk m_thunk;
    WNDPROC m_pfnSuperWindowProc;
    CDialogImplBaseT():TBase(),m_pfnSuperWindowProc(0) { }
    static LRESULT CALLBACK StartDialogProc(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
        CDialogImplBaseT *self;
        if (message==WM_INITDIALOG) {
            self=reinterpret_cast<CDialogImplBaseT*>(lparam);
            if (!self) return FALSE;
            self->m_hWnd=window;
            ::SetWindowLongPtrA(window,DWLP_USER,reinterpret_cast<LONG_PTR>(self));
        } else self=reinterpret_cast<CDialogImplBaseT*>(::GetWindowLongPtrA(window,DWLP_USER));
        if (!self) return FALSE;
        LRESULT result=0;
        BOOL handled=self->ProcessWindowMessage(window,message,wparam,lparam,result);
        if (message==WM_NCDESTROY) {
            ::SetWindowLongPtrA(window,DWLP_USER,0);
            self->m_hWnd=0;
        }
        if (!handled) return FALSE;
        switch(message) {
        case WM_INITDIALOG:case WM_CHARTOITEM:case WM_COMPAREITEM:
        case WM_VKEYTOITEM:case WM_QUERYDRAGICON:
        case WM_CTLCOLORMSGBOX:case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:case WM_CTLCOLORDLG:case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
            return result;
        default:
            ::SetWindowLongPtrA(window,DWLP_MSGRESULT,result);
            return TRUE;
        }
    }
};
template<class T,class TBase=CWindow> class ATL_NO_VTABLE CDialogImpl : public CDialogImplBaseT<TBase> {
public:
    INT_PTR DoModal(HWND parent=0,LPARAM initial=0) {
        (void)initial;
        return ::DialogBoxParamA(_Module.GetResourceInstance(),MAKEINTRESOURCEA(T::IDD),parent,
            reinterpret_cast<DLGPROC>(&CDialogImplBaseT<TBase>::StartDialogProc),
            reinterpret_cast<LPARAM>(this));
    }
};
typedef char dialog_base_size_is_28[(sizeof(CDialogImplBaseT<CWindow>)==28)?1:-1];
}
#define BEGIN_MSG_MAP(theClass) \
    virtual BOOL ProcessWindowMessage(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT& lResult,DWORD dwMsgMapID=0) { \
        (void)hWnd;(void)dwMsgMapID;BOOL bHandled=TRUE;
#define MESSAGE_HANDLER(message,handler) \
    if(uMsg==(message)){bHandled=TRUE;lResult=handler(uMsg,wParam,lParam,bHandled);if(bHandled)return TRUE;}
#define COMMAND_ID_HANDLER(id,handler) \
    if(uMsg==WM_COMMAND && LOWORD(wParam)==(id)){bHandled=TRUE;lResult=handler(HIWORD(wParam),LOWORD(wParam),(HWND)lParam,bHandled);if(bHandled)return TRUE;}
#define COMMAND_HANDLER(id,code,handler) \
    if(uMsg==WM_COMMAND && LOWORD(wParam)==(id) && HIWORD(wParam)==(code)){bHandled=TRUE;lResult=handler(HIWORD(wParam),LOWORD(wParam),(HWND)lParam,bHandled);if(bHandled)return TRUE;}
#define END_MSG_MAP() return FALSE; }
#endif
