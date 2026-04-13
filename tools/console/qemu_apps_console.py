import curses
import os
import re
import threading
import time
import sys
import select
from collections import deque

def select_pts():
    pts_list = [f"/dev/pts/{n}" for n in os.listdir("/dev/pts") if n.isdigit()]
    if not pts_list:
        print("[ERROR] No /dev/pts/N devices found.", file=sys.stderr)
        sys.exit(1)
    print("Available PTS devices:")
    for idx, path in enumerate(pts_list):
        print(f"  [{idx}] {path}")
    print("Press Enter with no input or Ctrl+C to exit.")
    while True:
        try:
            sel = input(f"Select PTS device [0-{len(pts_list)-1}]: ")
            if sel.strip() == "":
                print("Exiting PTS selection.")
                sys.exit(0)
            sel = int(sel)
            if 0 <= sel < len(pts_list):
                return pts_list[sel]
        except (ValueError, EOFError):
            print("Invalid selection. Try again.")
        except KeyboardInterrupt:
            print("\nExiting PTS selection.")
            sys.exit(0)


# --- Auto-detect new PTY if not provided ---

def list_pts():
    return set(f"/dev/pts/{n}" for n in os.listdir("/dev/pts") if n.isdigit())

def find_first_active_new_pts(before, timeout=120):
    start = time.time()
    while time.time() - start < timeout:
        after = list_pts()
        new_pts = sorted(after - before)
        if new_pts:
            # Try each new PTY for traffic
            for pts in new_pts:
                try:
                    fd = os.open(pts, os.O_RDWR | os.O_NONBLOCK)
                    r, _, _ = select.select([fd], [], [], 0.1)
                    if r:
                        # Read a small chunk to confirm traffic
                        try:
                            data = os.read(fd, 1024)
                            if data:
                                os.close(fd)
                                return pts
                        except Exception:
                            pass
                    os.close(fd)
                except Exception:
                    pass
        time.sleep(0.2)
    return None

if len(sys.argv) > 1:
    PTS_PATH = sys.argv[1]
    print(f"[INFO] Using provided PTY path: {PTS_PATH}")
    time.sleep(1)  # Give user a moment to read the message
    if PTS_PATH == "stdio":
        exit(0)
    elif not PTS_PATH.startswith("/dev/pts/"):
        PTS_PATH = None
    elif not os.path.exists(PTS_PATH):
        print(f"[WARN] Device {PTS_PATH} does not exist.")
        PTS_PATH = select_pts()
        print(f"Using {PTS_PATH}")

else:

    #wait until file is created
    while not os.path.exists('.out_select'):
        # print("[INFO] Waiting for .out_select file to be created by QEMU task...")
        time.sleep(1)
    with open('.out_select') as f:
        out_select = f.read().strip()
    # print(f"Output mode is: {out_select}")

    #delete the file to avoid confusion in future runs
    os.remove('.out_select')

    if out_select == "stdio":
        exit(0)

    print("[INFO] No PTY argument provided. Waiting for new PTY after QEMU starts...")
    before = list_pts()
    # print(f"[INFO] Existing PTYs before QEMU: {sorted(before)}")
    found = find_first_active_new_pts(before, timeout=600)
    if not found:
        print("[ERROR] No new PTY with traffic detected after waiting. Start QEMU first.", file=sys.stderr)
        sys.exit(1)
    print(f"[INFO] Detected active PTY: {found}")
    PTS_PATH = found

APP_PATTERNS = {
    "APP1": re.compile(r"^\[APP1\]"),
    "APP2": re.compile(r"^\[APP2\]"),
    "APP3": re.compile(r"^\[APP3\]"),
    "APP4": re.compile(r"^\[APP4\]"),
}

MAX_LINES = 200
SCROLL_STEP = 1
PAGE_STEP = 10


class PaneBuffer:
    def __init__(self, max_lines=MAX_LINES):
        self.lines = deque(maxlen=max_lines)
        self.lock = threading.Lock()
        self.scroll = 0  # 0 = follow newest output

    def append(self, line: str) -> None:
        with self.lock:
            at_bottom = (self.scroll == 0)
            self.lines.append(line.rstrip("\n"))
            if at_bottom:
                self.scroll = 0
            else:
                # If not at bottom, increment scroll to keep visible lines fixed
                max_scroll = max(0, len(self.lines) - 1)
                self.scroll = min(self.scroll + 1, max_scroll)

    def snapshot(self):
        with self.lock:
            return list(self.lines), self.scroll

    def scroll_up(self, n=1):
        with self.lock:
            max_scroll = max(0, len(self.lines) - 1)
            self.scroll = min(self.scroll + n, max_scroll)

    def scroll_down(self, n=1):
        with self.lock:
            self.scroll = max(self.scroll - n, 0)

    def scroll_to_bottom(self):
        with self.lock:
            self.scroll = 0


buffers = {
    "APP1": PaneBuffer(),
    "APP2": PaneBuffer(),
    "APP3": PaneBuffer(),
    "APP4": PaneBuffer(),
    "OTHER": PaneBuffer(),
}

pane_order = ["APP1", "APP2", "APP3", "APP4"]

input_buffer = ""
input_lock = threading.Lock()
stop_event = threading.Event()


def classify_line(line: str) -> str:
    for app_name, pattern in APP_PATTERNS.items():
        if pattern.match(line):
            return app_name
    return "OTHER"


def reader_thread():
    fd = os.open(PTS_PATH, os.O_RDWR | os.O_NOCTTY)

    partial = b""
    try:
        while not stop_event.is_set():
            try:
                chunk = os.read(fd, 1024)
                if not chunk:
                    time.sleep(0.01)
                    continue

                partial += chunk

                while b"\n" in partial:
                    raw_line, partial = partial.split(b"\n", 1)
                    line = raw_line.decode("utf-8", errors="replace")
                    target = classify_line(line)
                    buffers[target].append(line)

            except BlockingIOError:
                time.sleep(0.01)
            except OSError as e:
                buffers["OTHER"].append(f"[READER ERROR] {e}")
                break

        if partial:
            line = partial.decode("utf-8", errors="replace")
            target = classify_line(line)
            buffers[target].append(line)
    finally:
        os.close(fd)


def writer_send(text: str):
    fd = os.open(PTS_PATH, os.O_RDWR | os.O_NOCTTY)
    try:
        os.write(fd, text.encode("utf-8"))
    finally:
        os.close(fd)


def draw_box(win, title: str, active=False, scroll=0):
    win.box()
    prefix = "*" if active else " "
    label = f"{prefix} {title} "
    try:
        win.addstr(0, 2, label)
    except curses.error:
        pass

    if scroll > 0:
        indicator = f"[{scroll}]"
        try:
            _, width = win.getmaxyx()
            x = max(1, width - len(indicator) - 2)
            win.addnstr(0, x, indicator, len(indicator))
        except curses.error:
            pass


def render_pane(win, title: str, lines, scroll: int, active=False):
    win.erase()
    draw_box(win, title, active=active, scroll=scroll)

    height, width = win.getmaxyx()
    usable_h = max(0, height - 2)
    usable_w = max(0, width - 2)

    if usable_h <= 0 or usable_w <= 0:
        win.noutrefresh()
        return

    total = len(lines)

    if scroll == 0:
        start = max(0, total - usable_h)
    else:
        end = max(0, total - scroll)
        start = max(0, end - usable_h)

    visible_lines = lines[start:start + usable_h]

    for i, line in enumerate(visible_lines, start=1):
        try:
            win.addnstr(i, 1, line, usable_w)
        except curses.error:
            pass

    win.noutrefresh()


def ui_main(stdscr):
    global input_buffer

    active_pane = "APP1"

    curses.curs_set(1)
    stdscr.nodelay(True)
    stdscr.keypad(True)

    while not stop_event.is_set():
        h, w = stdscr.getmaxyx()

        if h < 8 or w < 20:
            stdscr.erase()
            try:
                stdscr.addstr(0, 0, "Terminal too small")
            except curses.error:
                pass
            stdscr.refresh()
            time.sleep(0.1)
            continue

        top_h = max(3, (h - 3) // 2)
        bottom_h = h - top_h - 3
        left_w = w // 2
        right_w = w - left_w

        app1_win = curses.newwin(top_h, left_w, 0, 0)
        app2_win = curses.newwin(top_h, right_w, 0, left_w)
        app3_win = curses.newwin(bottom_h, left_w, top_h, 0)
        app4_win = curses.newwin(bottom_h, right_w, top_h, left_w)
        input_win = curses.newwin(3, w, h - 3, 0)

        lines, scroll = buffers["APP1"].snapshot()
        render_pane(app1_win, "APP1", lines, scroll, active=(active_pane == "APP1"))

        lines, scroll = buffers["APP2"].snapshot()
        render_pane(app2_win, "APP2", lines, scroll, active=(active_pane == "APP2"))

        lines, scroll = buffers["APP3"].snapshot()
        render_pane(app3_win, "APP3", lines, scroll, active=(active_pane == "APP3"))

        lines, scroll = buffers["APP4"].snapshot()
        render_pane(app4_win, "APP4", lines, scroll, active=(active_pane == "APP4"))

        input_win.erase()
        draw_box(input_win, "INPUT")
        help_text = "TAB switch pane | ↑↓ scroll | PgUp/PgDn page | End live | Enter send | Esc quit"
        try:
            input_win.addnstr(0, max(1, w - len(help_text) - 2), help_text, max(0, w - 4))
        except curses.error:
            pass

        with input_lock:
            shown = input_buffer[-max(0, (w - 4)):]
        try:
            input_win.addnstr(1, 1, shown, max(0, w - 2))
            input_win.move(1, min(w - 2, 1 + len(shown)))
        except curses.error:
            pass
        input_win.noutrefresh()

        curses.doupdate()

        try:
            ch = stdscr.getch()
        except curses.error:
            ch = -1

        if ch == -1:
            time.sleep(0.03)
            continue

        if ch == 27:  # ESC
            stop_event.set()
            break

        elif ch == 9:  # TAB
            idx = pane_order.index(active_pane)
            active_pane = pane_order[(idx + 1) % len(pane_order)]

        elif ch in (curses.KEY_UP,):
            buffers[active_pane].scroll_up(SCROLL_STEP)

        elif ch in (curses.KEY_DOWN,):
            buffers[active_pane].scroll_down(SCROLL_STEP)

        elif ch == curses.KEY_PPAGE:  # Page Up
            buffers[active_pane].scroll_up(PAGE_STEP)

        elif ch == curses.KEY_NPAGE:  # Page Down
            buffers[active_pane].scroll_down(PAGE_STEP)

        elif ch == curses.KEY_HOME:
            lines, current_scroll = buffers[active_pane].snapshot()
            buffers[active_pane].scroll_up(len(lines) + current_scroll)

        elif ch == curses.KEY_END:
            buffers[active_pane].scroll_to_bottom()

        elif ch in (10, 13):  # Enter
            with input_lock:
                text = input_buffer
                input_buffer = ""
            writer_send(text + "\n")

        elif ch in (curses.KEY_BACKSPACE, 127, 8):
            with input_lock:
                input_buffer = input_buffer[:-1]

        elif 32 <= ch <= 126:
            with input_lock:
                input_buffer += chr(ch)


def main():

    t = threading.Thread(target=reader_thread, daemon=True)
    t.start()

    try:
        curses.wrapper(ui_main)
    finally:
        stop_event.set()
        t.join(timeout=1.0)


if __name__ == "__main__":
    main()