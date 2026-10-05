#Requires AutoHotkey v2.0.28
#SingleInstance Off
#Warn All, StdOut
#Warn LocalSameAsGlobal, Off
#Include controls.ahk
#Include actions.ahk
#Include steps.ahk

global GamePid := 0, StateHwnd := 0, Stage := A_Args[1], Artifacts := A_Args[2]
global UntilYear := Integer(A_Args[3]), Scenario := A_Args[4], StepName := "startup", StepCount := 0
global Continuation := Integer(A_Args[5]), KeepGame := Integer(A_Args[6])
global SeenPages := Map(), StepVisits := Map(), StartTime := A_TickCount
DetectHiddenWindows true
SetTitleMatchMode 2
SendMode "Event"
SetKeyDelay 10, 10
SetMouseDelay 10
SetWinDelay 0
; Keep modifiers held until Stars processes ControlClick's button messages.
SetControlDelay 20
CoordMode "Mouse", "Screen"

try {
    LogEvent("startup", "AutoHotkey " A_AhkVersion)
    if A_AhkVersion != "2.0.28"
        throw Error("Expected pinned AutoHotkey 2.0.28, got " A_AhkVersion)
    if Continuation {
        StateHwnd := WinWait("Stars Tutorial Test State ahk_class StarsTutorialObserver", , 5)
        if !StateHwnd
            throw Error("No retained live tutorial game in this Wine prefix")
        GamePid := WinGetPID(StateHwnd)
    } else {
        Run('"' Stage '\stars.exe"', Stage, , &GamePid)
        StateHwnd := WinWait("Stars Tutorial Test State ahk_pid " GamePid, , 20)
    }
    if !StateHwnd
        throw Error("No tutorial observer; build with STARS_TEST_TUTORIAL=ON")
    if Continuation {
        DismissPopup()
        if ReadState()["active"] != "1"
            throw Error("Retained game is not in tutorial mode")
        ConfigureWindows(ReadState())
        LogEvent("continuation", "Attached to a retained live game; this is development coverage")
    } else
        BeginTutorial()
    if Scenario = "reject-generate" {
        RejectGenerate()
        Finish("passed", "Premature generation rejected and tutorial remains usable")
    }
    Loop {
        state := ReadState()
        if state["autoComplete"] != "0"
            throw Error("Panic auto-completion was used")
        if state["active"] = "0" {
            if state["turn"] < 37
                throw Error("Tutorial exited before the final turn")
            if Continuation
                Finish("partial", "Live continuation reached the final year; a fresh full run is still required")
            Loop 80
                if !SeenPages.Has(A_Index)
                    throw Error("Tutorial page " A_Index " was not covered")
            Finish("passed", "All 80 pages completed through year 2437")
        }
        if Integer(state["year"]) >= UntilYear && UntilYear < 2437
            Finish("partial", "Stopped at requested development year " UntilYear)
        SeenPages[Integer(state["page"])] := true
        bold := Integer(state["bold"])
        instruction := IniRead(Stage "\instructions.ini", "Instructions", bold)
        StepName := "Year " state["year"] ", page " state["page"] ", instruction " bold ": " instruction
        visitKey := state["turn"] "|" bold
        StepVisits[visitKey] := StepVisits.Get(visitKey, 0) + 1
        if StepVisits[visitKey] > 6
            throw Error("Instruction repeated more than six times in the same year")
        StepCount += 1
        if StepCount > 1500
            throw Error("Exceeded 1500 actions without completing the tutorial")
        LogEvent("begin", StepName)
        LogEvent("state-before", "sel=" state["selection"] " scan=" state.Get("scanSelection", "?") " shift=" state.Get("keyShift", "?"))
        before := state["turn"] "|" state["page"] "|" state["bold"] "|" state["selection"] "|" state["message"]
        RunInstruction(bold, state)
        Sleep 50
        CheckDialogs()
        after := ReadState()
        LogEvent("end", StepName)
        LogEvent("state-after", "sel=" after["selection"] " scan=" after.Get("scanSelection", "?") " shift=" after.Get("keyShift", "?"))
        FileOpen(Artifacts "\last-state.txt", "w", "UTF-8-RAW").Write(WinGetText(StateHwnd))
        if before = after["turn"] "|" after["page"] "|" after["bold"] "|" after["selection"] "|" after["message"] {
            ; Some instruction sequences need several UI actions; their handlers
            ; assert their own changes. A repeated instruction still has a bound.
            if IsSet(previousBold) && previousBold = bold
                throw Error("Instruction did not advance after two attempts")
        }
        previousBold := bold
    }
} catch Error as failure {
    if GamePid
        try DismissPopup()
    try CaptureDesktop(Artifacts "\screenshots\failure.bmp")
    catch Error as captureError
        LogEvent("capture-error", captureError.Message)
    try DumpWindows()
    try FileOpen(Artifacts "\last-state.txt", "w", "UTF-8-RAW").Write(WinGetText(StateHwnd))
    Finish("failed", failure.Message "`n" failure.Stack)
}

; JsonString escapes text for JSON logs and results.
JsonString(value) {
    value := StrReplace(value, "\", "\\")
    value := StrReplace(value, '"', '\"')
    value := StrReplace(value, "`r", "\r")
    value := StrReplace(value, "`n", "\n")
    value := StrReplace(value, "`t", "\t")
    return '"' value '"'
}

; LogEvent records each action with its elapsed time and human-readable name.
LogEvent(event, detail) {
    global Artifacts, StartTime
    FileAppend('{"elapsed_ms":' (A_TickCount - StartTime) ',"event":' JsonString(event) ',"detail":' JsonString(detail) '}`n', Artifacts "\events.jsonl", "UTF-8-RAW")
}

; Finish publishes an atomic result and closes only the launched game process.
Finish(status, detail) {
    global Artifacts, GamePid, StepName, StepCount, SeenPages, KeepGame
    text := '{"status":' JsonString(status) ',"detail":' JsonString(detail) ',"step":' JsonString(StepName) ',"actions":' StepCount ',"pages":' SeenPages.Count '}'
    FileOpen(Artifacts "\result.tmp", "w", "UTF-8-RAW").Write(text)
    xml := StrReplace(StrReplace(StrReplace(detail, "&", "&amp;"), "<", "&lt;"), '"', "&quot;")
    body := status = "passed" ? "" : '<' (status = "partial" ? "skipped" : "failure") ' message="' xml '"/>'
    FileOpen(Artifacts "\results.xml", "w", "UTF-8-RAW").Write('<testsuite name="tutorial" tests="1" failures="' (status = "failed" ? 1 : 0) '" skipped="' (status = "partial" ? 1 : 0) '"><testcase name="' Scenario '">' body '</testcase></testsuite>')
    FileMove Artifacts "\result.tmp", Artifacts "\result.json", true
    if status != "failed" || !KeepGame
        try ProcessClose GamePid
    ExitApp status = "passed" ? 0 : 1
}

; DumpWindows saves runtime window identities and control inventories on failure.
DumpWindows() {
    global GamePid, Artifacts
    output := ""
    for hwnd in WinGetList("ahk_pid " GamePid) {
        output .= hwnd " visible=" DllCall("IsWindowVisible", "ptr", hwnd) " enabled=" DllCall("IsWindowEnabled", "ptr", hwnd) " " WinGetClass(hwnd) " " WinGetTitle(hwnd) "`n" WinGetText(hwnd) "`n"
        for control in WinGetControlsHwnd(hwnd) {
            output .= "  " control " id=" DllCall("GetDlgCtrlID", "ptr", control, "int") " " WinGetClass(control) " " ControlGetText(control) "`n"
        }
    }
    FileOpen(Artifacts "\windows.txt", "w", "UTF-8-RAW").Write(output)
}

; CaptureDesktop writes a BMP from the Wine desktop using Win32 GDI.
CaptureDesktop(path) {
    width := DllCall("GetSystemMetrics", "int", 0, "int")
    height := DllCall("GetSystemMetrics", "int", 1, "int")
    dc := DllCall("GetDC", "ptr", 0, "ptr")
    memory := DllCall("gdi32\CreateCompatibleDC", "ptr", dc, "ptr")
    bitmap := DllCall("gdi32\CreateCompatibleBitmap", "ptr", dc, "int", width, "int", height, "ptr")
    old := DllCall("gdi32\SelectObject", "ptr", memory, "ptr", bitmap, "ptr")
    try {
        if !DllCall("gdi32\BitBlt", "ptr", memory, "int", 0, "int", 0, "int", width, "int", height, "ptr", dc, "int", 0, "int", 0, "uint", 0x00CC0020)
            if !DllCall("PrintWindow", "ptr", Integer(ReadState()["hwnd.frame"]), "ptr", memory, "uint", 0)
                throw Error("Wine driver cannot capture desktop or print the game window")
        DllCall("gdi32\SelectObject", "ptr", memory, "ptr", old)
        info := Buffer(40, 0), pixels := Buffer(width * height * 4)
        NumPut("uint", 40, "int", width, "int", height, "ushort", 1, "ushort", 32, info)
        if !DllCall("gdi32\GetDIBits", "ptr", dc, "ptr", bitmap, "uint", 0, "uint", height, "ptr", pixels, "ptr", info, "uint", 0)
            throw Error("Cannot read captured desktop pixels")
        nonblank := false
        Loop pixels.Size // 4096
            if NumGet(pixels, (A_Index - 1) * 4096, "uint") & 0xFFFFFF {
                nonblank := true
                break
            }
        if !nonblank
            throw Error("Wine driver returned a blank capture; use an X11 display for screenshots")
        header := Buffer(14, 0)
        NumPut("ushort", 0x4D42, "uint", 54 + pixels.Size, header)
        NumPut("uint", 54, header, 10)
        file := FileOpen(path, "w")
        file.RawWrite(header), file.RawWrite(info), file.RawWrite(pixels)
        file.Close()
    } finally {
        DllCall("gdi32\DeleteObject", "ptr", bitmap)
        DllCall("gdi32\DeleteDC", "ptr", memory)
        DllCall("ReleaseDC", "ptr", 0, "ptr", dc)
    }
}
