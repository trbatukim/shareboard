#include <windows.h>
#include <cstdio>

static HHOOK g_keyboardHook = nullptr;
static HHOOK g_mouseHook = nullptr;

static const char* KeyboardEventName(WPARAM wParam) {
    switch (wParam) {
        case WM_KEYDOWN:    return "KEYDOWN";
        case WM_KEYUP:      return "KEYUP";
        case WM_SYSKEYDOWN: return "SYSKEYDOWN";  // Alt
        case WM_SYSKEYUP:   return "SYSKEYUP";
        default:            return "OTHER";
    }
}

static const char* MouseEventName(WPARAM wParam) {
    switch (wParam) {
        case WM_MOUSEMOVE:      return "MOUSEMOVE";
        case WM_LBUTTONDOWN:    return "LBUTTONDOWN";
        case WM_LBUTTONUP:      return "LBUTTONUP";
        case WM_RBUTTONDOWN:    return "RBUTTONDOWN";
        case WM_RBUTTONUP:      return "RBUTTONUP";
        case WM_MBUTTONDOWN:    return "MBUTTONDOWN";
        case WM_MBUTTONUP:      return "MBUTTONUP";
        case WM_XBUTTONDOWN:    return "XBUTTONDOWN";
        case WM_XBUTTONUP:      return "XBUTTONUP";
        case WM_MOUSEWHEEL:     return "MOUSEWHEEL";
        case WM_MOUSEHWHEEL:    return "MOUSEHWHEEL";
        default:                return "OTHER";
    }
}

static void FormatMouseDetail(WPARAM wParam, short high, char* out, size_t n) {
    out[0] = '\0';

    switch (wParam) {
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            snprintf(out, n, "delta=%d", high);
            break;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            snprintf(out, n, "xbutton=%d", high);
            break;
        default:
            break;
    }
}

static bool IsQuitCombo(const KBDLLHOOKSTRUCT* key, WPARAM wParam) {
    if (wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN) return false;
    if (key->vkCode != 'Q') return false;

    // High bit set means the key is down right now.
    const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt  = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    return ctrl && alt;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        const KBDLLHOOKSTRUCT* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);

        // LLKHF_INJECTED marks events produced by SendInput rather than real
        // hardware. Machine B replays our events with SendInput, so once this
        // is bidirectional we must ignore injected events or the two machines
        // will echo each other forever.
        const bool injected = (key->flags & LLKHF_INJECTED) != 0;

        printf("%-11s vk=0x%02X scan=0x%02X %s%s\n",
               KeyboardEventName(wParam),
               static_cast<unsigned>(key->vkCode),
               static_cast<unsigned>(key->scanCode),
               (key->flags & LLKHF_EXTENDED) ? "[ext] " : "",
               injected ? "[injected]" : "");

        if (IsQuitCombo(key, wParam)) {
            printf("\nCtrl+Alt+Q shutting down.\n");
            PostQuitMessage(0);
            return 1;
        }
    }

    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam != WM_MOUSEMOVE) {
        const MSLLHOOKSTRUCT* mouse = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);

        // mouseData only carries a payload for wheels and X buttons, and it
        // sits in the HIGH word. Wheel deltas are signed multiples of 120
        const short high = static_cast<short>(HIWORD(mouse->mouseData));

        char detail[24];
        FormatMouseDetail(wParam, high, detail, sizeof(detail));

        printf("%-11s %-12s %d,%d %s%s\n",
               MouseEventName(wParam),
               detail,
               mouse->pt.x, mouse->pt.y,
               (mouse->flags & LLMHF_INJECTED) ? "[injected]" : "",
               (mouse->flags & LLMHF_LOWER_IL_INJECTED) ? "[lower-il]" : "");
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

int main() {
    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);

    if (!g_keyboardHook){
        printf("SetWindowsHookExW failed: %lu\n", GetLastError());
        return 1;
    }

    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, nullptr, 0);

    if (!g_mouseHook){
        printf("SetWindowsHookExW failed: %lu\n", GetLastError());
        UnhookWindowsHookEx(g_keyboardHook);
        return 1;
    }

    printf("Keyboard and mouse hooks installed. Press keys to see them; Ctrl+Alt+Q to quit.\n\n");

    MSG msg;
    BOOL result;
    while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
        if (result == -1) break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_keyboardHook);
    UnhookWindowsHookEx(g_mouseHook);
    return 0;
}
