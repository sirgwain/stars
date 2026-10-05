; ReadState reads a complete snapshot from the test-only observer.
ReadState() {
    global StateHwnd, GamePid, SeenPages
    if !ProcessExist(GamePid)
        throw Error("Stars exited unexpectedly")
    SendMessage 0x8001, 0, 0, , StateHwnd, , , , 5000
    raw := ControlGetText(DllCall("GetDlgItem", "ptr", StateHwnd, "int", 50000, "ptr"))
    state := Map()
    for line in StrSplit(raw, "`n", "`r") {
        separator := InStr(line, "=")
        if separator
            state[SubStr(line, 1, separator - 1)] := SubStr(line, separator + 1)
    }
    if !state.Has("version") || state["version"] != "1"
        throw Error("Missing or unsupported tutorial snapshot")
    if state["active"] = "1"
        SeenPages[Integer(state["page"])] := true
    return state
}

; WaitFor waits for an observable state condition and rejects unexpected dialogs.
WaitFor(predicate, description, seconds := 10) {
    deadline := A_TickCount + seconds * 1000
    Loop {
        CheckDialogs()
        state := ReadState()
        if predicate(state)
            return state
        if A_TickCount >= deadline
            throw Error("Timed out waiting for " description)
        Sleep 100
    }
}

; CheckDialogs fails on error message boxes instead of dismissing regressions.
CheckDialogs() {
    global GamePid
    for hwnd in WinGetList("ahk_class #32770 ahk_pid " GamePid) {
        if !DllCall("IsWindowVisible", "ptr", hwnd)
            continue
        title := WinGetTitle(hwnd)
        text := WinGetText(hwnd)
        if title = "Stars!" && (InStr(text, "Tutorial:") || InStr(text, "error") || InStr(text, "unable") || InStr(text, "corrupt"))
            throw Error("Unexpected game dialog: " text)
    }
}

; WaitDialog finds a visible game dialog by its runtime title within a bounded wait.
WaitDialog(title, seconds := 10) {
    global GamePid
    deadline := A_TickCount + seconds * 1000
    Loop {
        for hwnd in WinGetList(title " ahk_class #32770 ahk_pid " GamePid)
            if DllCall("IsWindowVisible", "ptr", hwnd) && (title != "Stars!" || WinGetTitle(hwnd) = title)
                return hwnd
        if A_TickCount >= deadline
            throw Error("Dialog did not open: " title)
        Sleep 100
    }
}

; SendModifier presses or releases Shift, Ctrl or Alt and waits until Wine
; reports the new state. Under Xvfb, Wine can deliver a click before the
; key event that preceded it, so the game saw shift-clicks as plain clicks
; and the next plain click as a shift-click.
SendModifier(modifier, down) {
    static vks := Map("Shift", 0x10, "Ctrl", 0x11, "Alt", 0x12)
    SendEvent "{" modifier (down ? " down}" : " up}")
    deadline := A_TickCount + 2000
    while ((DllCall("GetAsyncKeyState", "int", vks[modifier], "short") & 0x8000) != 0) != down {
        if A_TickCount > deadline
            throw Error(modifier (down ? " did not go down" : " did not come up"))
        Sleep 10
    }
}

; ClickControl clicks an initialized, visible and enabled control by HWND.
ClickControl(control, modifier := "") {
    control := Integer(control)
    if !control || !DllCall("IsWindowVisible", "ptr", control) || !ControlGetEnabled(control)
        throw Error("Control is unavailable: " control)
    owner := DllCall("GetAncestor", "ptr", control, "uint", 2, "ptr")
    WinActivate owner
    if !WinWaitActive(owner, , 3)
        throw Error("Cannot activate control's window")
    try {
        if modifier != "" {
            SendModifier(modifier, true)
            Sleep 50
        }
        ControlClick control
    } finally {
        if modifier != ""
            SendModifier(modifier, false)
    }
    Sleep 100
    ReadState()
}

; ClickId resolves a dialog resource control ID and clicks its current HWND.
ClickId(dialog, id, modifier := "") {
    control := DllCall("GetDlgItem", "ptr", dialog, "int", id, "ptr")
    ClickControl(control, modifier)
}

; Choose selects a named entry through the control's normal selection notification.
Choose(control, name) {
    if !control || !DllCall("IsWindowVisible", "ptr", Integer(control))
        throw Error("Selection control is unavailable")
    items := ControlGetItems(Integer(control))
    composition := Integer(ReadState()["hwnd.composition"])
    index := 0
    for candidate, item in items {
        if DllCall("GetDlgCtrlID", "ptr", Integer(control), "int") = Controls["IDC_DESIGNER_COMPONENT_LIST"] {
            ; The owner-drawn component list stores four metadata bytes before its display name.
            if StrLen(item) < 5
                throw Error("Component list entry has no display name")
            visibleName := Trim(SubStr(item, 5))
        } else if Integer(control) = composition {
            ; Fleet rows contain two icon bytes and a five-character ship count.
            if StrLen(item) < 8
                throw Error("Fleet composition entry has no design name")
            visibleName := SubStr(item, 8)
        } else
            visibleName := RegExReplace(Trim(item), "^(?:[\*#]\s*|I\s{2,})")
        if visibleName = name {
            index := candidate
            break
        }
    }
    if !index
        throw Error("List has no exact entry '" name "'; items: " Join(items, "; "))
    SelectIndex(Integer(control), index)
    Sleep 50
}

; Join formats available choices in diagnostics.
Join(items, separator) {
    result := ""
    for index, item in items
        result .= (index > 1 ? separator : "") item
    return result
}

; Keys sends actual keyboard input to the main game window.
Keys(keys) {
    frame := Integer(ReadState()["hwnd.frame"])
    WinActivate frame
    if !WinWaitActive(frame, , 3)
        throw Error("Cannot activate Stars")
    SendEvent keys
    Sleep 50
    ReadState()
}

; ClientPoint converts a point in a specific surface to desktop coordinates.
ClientPoint(hwnd, x, y) {
    point := Buffer(8)
    NumPut("int", Integer(x), "int", Integer(y), point)
    if !DllCall("ClientToScreen", "ptr", Integer(hwnd), "ptr", point)
        throw Error("Cannot translate client coordinates")
    return [NumGet(point, 0, "int"), NumGet(point, 4, "int")]
}

; ClickPoint performs a real mouse gesture at a named surface's client point.
ClickPoint(hwnd, x, y, modifier := "", button := "Left", count := 1) {
    global GamePid
    owner := DllCall("GetAncestor", "ptr", Integer(hwnd), "uint", 2, "ptr")
    WinActivate owner
    if !WinWaitActive(owner, , 3) {
        active := WinExist("A")
        if !DllCall("IsWindowEnabled", "ptr", owner) || !active || WinGetPID(active) != GamePid
            throw Error("Cannot activate mouse target window")
        LogEvent("input-focus", "An enabled game-owned dialog retained focus before the mouse gesture")
    }
    point := ClientPoint(hwnd, x, y)
    hitPoint := Buffer(8)
    NumPut("int", point[1], "int", point[2], hitPoint)
    hit := DllCall("WindowFromPoint", "int64", NumGet(hitPoint, 0, "int64"), "ptr")
    if hit != Integer(hwnd) && !DllCall("IsChild", "ptr", Integer(hwnd), "ptr", hit)
        throw Error("Click target is covered: " WinGetClass(hit) " " WinGetTitle(hit))
    try {
        if modifier != "" {
            SendModifier(modifier, true)
            Sleep 50
        }
        MouseClick button, point[1], point[2], count, 0
        if modifier != ""
            Sleep 100
    } finally {
        if modifier != ""
            SendModifier(modifier, false)
    }
    Sleep 50
    ReadState()
}

; ClickRect clicks the center of a rectangle published by the actual layout.
ClickRect(key, modifier := "", button := "Left") {
    rect := StrSplit(ReadState()[key], "|")
    if Integer(rect[4]) <= Integer(rect[2]) || Integer(rect[5]) <= Integer(rect[3])
        throw Error("Named rectangle is unavailable: " key)
    ClickPoint(rect[1], (Integer(rect[2]) + Integer(rect[4])) // 2, (Integer(rect[3]) + Integer(rect[5])) // 2, modifier, button)
}

; ObjectPoint resolves a planet or fleet name using current scanner coordinates.
ObjectPoint(name, kind := "planet") {
    Loop 80 {
        state := ReadState()
        for key, value in state {
            if SubStr(key, 1, StrLen(kind) + 1) != kind "."
                continue
            parts := StrSplit(value, "|")
            if parts[1] != name
                continue
            hwnd := Integer(state["hwnd.scanner"])
            x := Integer(parts[2]), y := Integer(parts[3])
            viewport := StrSplit(state["scannerViewport.0"], "|")
            bounds := StrSplit(state["scannerBounds.0"], "|")
            width := Integer(bounds[4]), height := Integer(bounds[5])
            if x >= 5 && x < Integer(viewport[4]) - 5 && y >= 5 && y < Integer(viewport[5]) - 5
                return [hwnd, x, y, Integer(SubStr(key, StrLen(kind) + 2))]
            if y < 5 || y >= Integer(viewport[5]) - 5 {
                arrow := DllCall("GetSystemMetrics", "int", 2, "int")
                ClickPoint(hwnd, width + arrow // 2, y < 5 ? arrow // 2 : height - arrow // 2)
            } else {
                arrow := DllCall("GetSystemMetrics", "int", 3, "int")
                ClickPoint(hwnd, x < 5 ? arrow // 2 : width - arrow // 2, height + arrow // 2)
            }
            break
        }
        if !IsSet(parts) || parts[1] != name
            throw Error("Unknown " kind ": " name)
    }
    throw Error("Cannot scroll " kind " into scanner viewport: " name)
}

; PlanetClick selects or assigns a destination through the scanner.
PlanetClick(name, modifier := "", count := 1) {
    point := ObjectPoint(name)
    ClickPoint(point[1], point[2], point[3], modifier, "Left", count)
}

; Find opens View/Find and selects a fleet or planet by its displayed name.
Find(name) {
    Keys("!vf")
    dialog := WaitDialog("Find Planet or Fleet")
    control := DllCall("GetDlgItem", "ptr", dialog, "int", Controls["IDC_EDIT1"], "ptr")
    ControlSetText name, control
    ClickId(dialog, Controls["IDOK"])
    if !WinWaitClose(dialog, , 5)
        throw Error("Find dialog did not close for " name)
}

; PlanetSelect selects a named planet in the command pane through its scanner menu.
PlanetSelect(name) {
    point := ObjectPoint(name)
    ClickPoint(point[1], point[2], point[3], , "Right")
    PopupChoice(name)
    WaitFor((s) => StrSplit(s["selection"], "|")[1] = "1" && Integer(StrSplit(s["selection"], "|")[2]) = point[4], "command planet " name)
}

; NextMessage advances through the Messages pane's Next button.
NextMessage(goToObject := false) {
    state := ReadState()
    ClickControl(state["messageButton.2"])
    if goToObject
        MessageGoto()
}

; MessageGoto activates the current message's Goto or View action.
MessageGoto() {
    ClickControl(ReadState()["messageButton.1"])
}

; ReadMessages reads all remaining messages through the pane's Next button.
ReadMessages() {
    limit := Integer(ReadState()["messageCount"]) + 1
    Loop limit {
        next := Integer(ReadState()["messageButton.2"])
        if !ControlGetEnabled(next)
            return
        NextMessage()
    }
    throw Error("Messages did not reach their last entry")
}

; SetTask selects the waypoint task without writing game state directly.
SetTask(task) {
    Choose(ReadState()["orderControl.0"], task)
}

; Production adds a specified quantity and checks the live list before closing.
Production(item, quantity, leftover := -1, position := -1) {
    if !Integer(ReadState()["hwnd.production"])
        Keys("q")
    dialog := Integer(WaitFor((s) => Integer(s["hwnd.production"]) != 0, "production queue")["hwnd.production"])
    available := DllCall("GetDlgItem", "ptr", dialog, "int", Controls["IDC_PRODUCTION_AVAILABLE_ITEMS"], "ptr")
    queue := DllCall("GetDlgItem", "ptr", dialog, "int", Controls["IDC_PRODUCTION_QUEUE"], "ptr")
    Choose(available, item)
    SelectIndex(queue, position < 0 ? ControlGetItems(queue).Length : position + 1)
    hundreds := quantity // 100, tens := Mod(quantity, 100) // 10, ones := Mod(quantity, 10)
    Loop hundreds
        ClickId(dialog, Controls["IDC_PRODUCTION_ADD"], "Ctrl")
    Loop tens
        ClickId(dialog, Controls["IDC_PRODUCTION_ADD"], "Shift")
    Loop ones
        ClickId(dialog, Controls["IDC_PRODUCTION_ADD"])
    LogEvent("queue", Join(ControlGetItems(queue), "; "))
    if leftover >= 0 {
        check := DllCall("GetDlgItem", "ptr", dialog, "int", Controls["IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY"], "ptr")
        if ControlGetChecked(check) != leftover
            ClickControl(check)
    }
    ClickId(dialog, Controls["IDOK"])
    WinWaitClose dialog, , 5
}

; Research selects current and next fields through their dialog controls.
Research(current := "", next := "") {
    if !Integer(ReadState()["hwnd.research"])
        Keys("{F5}")
    dialog := WaitDialog("Research")
    fields := ["Energy", "Weapons", "Propulsion", "Construction", "Electronics", "Biotechnology"]
    if current != ""
        for index, field in fields
            if current = field
                ClickId(dialog, Controls["IDC_RESEARCH_CONSTRUCTION"] - 4 + index)
    if next != ""
        Choose(DllCall("GetDlgItem", "ptr", dialog, "int", Controls["IDC_RESEARCH_NEXT_FIELD"], "ptr"), next)
    ClickId(dialog, Controls["IDCANCEL"])
    WinWaitClose dialog, , 5
}

; BeginTutorial enters the tutorial through New Game and verifies its first page.
BeginTutorial() {
    deadline := A_TickCount + 20000
    Loop {
        state := ReadState()
        title := Integer(state["hwnd.title"])
        if title && DllCall("IsWindowVisible", "ptr", title) && DllCall("IsWindowEnabled", "ptr", title)
            break
        if A_TickCount >= deadline
            throw Error("Timed out waiting for the startup window")
        Sleep 100
    }
    ClickControl(ControlGetHwnd("&New Game...", title))
    dialog := WaitDialog("New Game")
    ClickId(dialog, Controls["IDC_SIMPLE_NEW_GAME_TUTORIAL"])
    WaitFor((s) => s["active"] = "1" && s["bold"] = "5", "first tutorial instruction", 30)
    state := ReadState()
    ConfigureWindows(state)
    try CaptureDesktop(Artifacts "\screenshots\start.bmp")
    catch Error as captureError
        LogEvent("capture-error", captureError.Message)
}

; Generate asserts the current tasks passed before invoking the next turn.
Generate() {
    state := ReadState()
    if state["turnDone"] != "1"
        throw Error("Tutorial tasks have not passed before Generate")
    FileOpen(Artifacts "\year-" state["year"] ".txt", "w", "UTF-8-RAW").Write(ControlGetText(DllCall("GetDlgItem", "ptr", StateHwnd, "int", 50000, "ptr")))
    turn := Integer(state["turn"])
    Keys("{F9}")
    if turn < 36
        WaitFor((s) => Integer(s["turn"]) = turn + 1, "year " (2401 + turn), 30)
    else {
        global GamePid
        deadline := A_TickCount + 30000
        Loop {
            for hwnd in WinGetList("Stars! ahk_class #32770 ahk_pid " GamePid)
                if InStr(WinGetText(hwnd), "The tutorial is finished.") {
                    LogEvent("completion", WinGetText(hwnd))
                    ClickId(hwnd, Controls["IDOK"])
                    WaitFor((s) => s["active"] = "0", "exit from tutorial mode")
                    return
                }
            if A_TickCount > deadline
                throw Error("Final tutorial completion message did not appear")
            Sleep 100
        }
    }
}

; RejectGenerate verifies an unfinished turn is rejected without advancing time.
RejectGenerate() {
    global GamePid
    Keys("{F9}")
    dialog := WaitDialog("Stars!")
    if !InStr(WinGetText(dialog), "You have not yet completed all of the tutorial tasks")
        throw Error("Expected the unfinished-tutorial rejection")
    ClickId(dialog, Controls["IDOK"])
    state := ReadState()
    if state["turn"] != "0" || state["active"] != "1" || state["autoComplete"] != "0"
        throw Error("Premature Generate altered tutorial state")
    ReadMessages()
    WaitFor((s) => s["page"] = "2", "tutorial recovery after rejected generation")
}

; ReadTo advances real messages to a required zero-based tutorial message index.
ReadTo(index, goToObject := false) {
    Loop Integer(ReadState()["messageCount"]) + 1 {
        current := Integer(ReadState()["message"])
        if current >= index {
            if goToObject
                MessageGoto()
            return
        }
        NextMessage()
    }
    throw Error("Required message was not reachable: " index)
}

; FleetSelect selects a fleet by its observed identity through the scanner menu.
FleetSelect(id) {
    parts := StrSplit(ReadState()["fleet." id], "|")
    point := ObjectPoint(parts[1], "fleet")
    ClickPoint(point[1], point[2], point[3], , "Right")
    PopupChoice(parts[1])
    WaitFor((s) => StrSplit(s["selection"], "|")[1] = "2" && Integer(StrSplit(s["selection"], "|")[2]) = id, "command fleet " id)
}

; FleetClick clicks a fleet's actual scanner location, including enemy targets.
FleetClick(id, modifier := "", count := 1) {
    parts := StrSplit(ReadState()["fleet." id], "|")
    point := ObjectPoint(parts[1], "fleet")
    ClickPoint(point[1], point[2], point[3], modifier, "Left", count)
}

; FilterMessage toggles the current message type's blue filter checkbox.
FilterMessage() {
    rect := StrSplit(ReadState()["messageTitle.0"], "|")
    size := Integer(rect[5]) - Integer(rect[3])
    ClickPoint(rect[1], Integer(rect[2]) + size // 2, Integer(rect[3]) + size // 2)
}

; Waypoint selects the actual list entry for a zero-based waypoint.
Waypoint(index) {
    hwnd := Integer(ReadState()["hwnd.waypoints"])
    if index >= ControlGetItems(hwnd).Length
        throw Error("Waypoint does not exist: " index)
    SelectIndex(hwnd, index + 1)
    Sleep 50
}

; DeleteWaypoint selects and deletes a waypoint using keyboard input.
DeleteWaypoint(fleet, index) {
    FleetSelect(fleet)
    Waypoint(index)
    hwnd := Integer(ReadState()["hwnd.waypoints"])
    ControlFocus hwnd
    ControlSend "{Delete}", hwnd
    Sleep 50
}

; DragWaypoint changes a destination through the scanner's drag gesture.
DragWaypoint(fleet, index, destination, kind := "planet") {
    FleetSelect(fleet)
    state := ReadState()
    if kind = "planet"
        target := ObjectPoint(destination)
    else {
        parts := StrSplit(state["fleet." destination], "|")
        target := [state["hwnd.scanner"], parts[2], parts[3]]
    }
    state := ReadState(), wp := StrSplit(state["waypoint." fleet "." index], "|")
    from := ClientPoint(state["hwnd.scanner"], wp[5], wp[6])
    to := ClientPoint(target[1], target[2], target[3])
    WinActivate Integer(state["hwnd.frame"])
    MouseClickDrag "Left", from[1], from[2], to[1], to[2], 2
    Sleep 100
}

; ScannerMode clicks the toolbar's normal or planet-value display button.
ScannerMode(mode) {
    ClickRect("toolbarButton." mode)
    WaitFor((s) => (Integer(s["scannerMode"]) & 15) = mode, "scanner display mode " mode)
}

; PopupChoice selects an exact native popup menu item by its visible text.
PopupChoice(name) {
    global GamePid
    menuWindow := WinWait("ahk_class #32768 ahk_pid " GamePid, , 3)
    if !menuWindow
        throw Error("Popup menu did not open for " name)
    state := WaitFor((s) => s.Has("menu.0"), "popup menu observations")
    choices := []
    for key, value in state {
        if SubStr(key, 1, 5) != "menu."
            continue
        parts := StrSplit(value, "|")
        text := Trim(StrReplace(parts[5], "&"))
        choices.Push(text)
        if text = name {
            x := (Integer(parts[1]) + Integer(parts[3])) // 2
            y := (Integer(parts[2]) + Integer(parts[4])) // 2
            point := Buffer(8)
            NumPut("int", x, "int", y, point)
            hit := DllCall("WindowFromPoint", "int64", NumGet(point, 0, "int64"), "ptr")
            LogEvent("menu-choice", name " at " x "," y " on " WinGetClass(hit))
            if WinGetClass(hit) != "#32768"
                throw Error("Popup item rectangle is not on a native menu: " name)
            ; Move directly into a submenu; a diagonal path can close its parent menu.
            MouseMove x, y, 0
            Sleep 50
            MouseClick "Left"
            Sleep 50
            return
        }
    }
    SendEvent "{Esc}"
    throw Error("Popup has no exact choice '" name "': " Join(choices, "; "))
}

; Transport configures colonist handling using the task's native dropdowns.
Transport(action) {
    SetTask("Transport")
    Choose(ReadState()["orderControl.1"], "Colonists")
    Choose(ReadState()["orderControl.2"], action)
}

; TransportPreset applies a named preset from the waypoint task diamond.
TransportPreset(name) {
    SetTask("Transport")
    ClickRect("reference.5", , "Right")
    PopupChoice(name)
}

; RepeatOrders enables the selected fleet's real repeat checkbox.
RepeatOrders() {
    hwnd := Integer(ReadState()["hwnd.repeat"])
    if !ControlGetChecked(hwnd)
        ClickControl(hwnd)
}

; Colonize assigns a named destination and its colonization task.
Colonize(fleet, planet, load := false) {
    FleetSelect(fleet)
    if load
        LoadColonists()
    PlanetClick(planet, "Shift")
    SetTask("Colonize")
}

; OpenDesigner opens or reuses the Ship and Starbase Designer.
OpenDesigner() {
    hwnd := Integer(ReadState()["hwnd.designer"])
    if !hwnd {
        Keys("{F4}")
        hwnd := WaitDialog("Ship & Starbase Designer")
    }
    return hwnd
}

; CopyHull starts a new design from an available hull.
CopyHull(hull, starbase := false) {
    hwnd := OpenDesigner()
    ClickId(hwnd, starbase ? Controls["IDC_DESIGNER_STARBASES"] : Controls["IDC_DESIGNER_SHIPS"])
    ClickId(hwnd, Controls["IDC_DESIGNER_HULLS"])
    Choose(DllCall("GetDlgItem", "ptr", hwnd, "int", Controls["IDC_COMBOBOX"], "ptr"), hull)
    ClickId(hwnd, Controls["IDC_IMPORT"])
    WaitFor((s) => Integer(s["buildMode"]) = 4, "editable hull")
}

; AddPart drags a named component into a slot and checks the resulting count.
AddPart(category, name, slot, count := 1) {
    hwnd := Integer(ReadState()["hwnd.designer"])
    Choose(DllCall("GetDlgItem", "ptr", hwnd, "int", Controls["IDC_COMBOBOX"], "ptr"), category)
    list := DllCall("GetDlgItem", "ptr", hwnd, "int", Controls["IDC_DESIGNER_COMPONENT_LIST"], "ptr")
    Choose(list, name)
    item := ControlGetIndex(list) - 1, rect := Buffer(16)
    if SendMessage(0x198, item, rect.Ptr, , list) = -1
        throw Error("Cannot locate component row " name)
    from := ClientPoint(list, 32, (NumGet(rect, 4, "int") + NumGet(rect, 12, "int")) // 2)
    destination := StrSplit(ReadState()["slot." slot], "|")
    to := ClientPoint(hwnd, (Integer(destination[2]) + Integer(destination[4])) // 2, (Integer(destination[3]) + Integer(destination[5])) // 2)
    Loop count {
        WinActivate hwnd
        MouseClickDrag "Left", from[1], from[2], to[1], to[2], 2
        ; FakeListProc handles button-down but not double-click messages.
        ; Repeated drags must start outside the system double-click interval.
        Sleep A_Index < count ? DllCall("GetDoubleClickTime", "uint") + 20 : 50
    }
    WaitFor((s) => Integer(StrSplit(s["component." slot], "|")[3]) >= count, "component " name " in slot " slot)
}

; FinishDesign accepts the edited design and closes the designer through Done.
FinishDesign(name := "", nextImage := false) {
    hwnd := Integer(ReadState()["hwnd.designer"])
    if Integer(ReadState()["buildMode"]) = 4 {
        if name != ""
            ControlSetText name, DllCall("GetDlgItem", "ptr", hwnd, "int", Controls["IDC_EDITNAME"], "ptr")
        if nextImage
            ClickRect("imageArrow.1")
        ClickId(hwnd, Controls["IDOK"])
        Sleep 50
    }
    ClickId(hwnd, Controls["IDCANCEL"])
    if !WinWaitClose(hwnd, , 5)
        throw Error("Designer did not close")
}

; ReportSort opens a report's live column menu and selects its sort entry.
ReportSort(column, name, sub := "") {
    if !Integer(ReadState()["hwnd.report"])
        Keys("!rp")
    state := WaitFor((s) => s.Has("reportColumn." column), "report column " column)
    WinMove 0, 0, 1280, 700, Integer(state["hwnd.report"])
    Sleep 50
    ClickRect("reportColumn." column, , "Right")
    PopupChoice(name)
    if sub != ""
        PopupChoice(sub)
}

; CloseReport closes the report through its real Escape shortcut.
CloseReport() {
    hwnd := Integer(ReadState()["hwnd.report"])
    WinActivate hwnd
    SendEvent "{Esc}"
    if !WinWaitClose(hwnd, , 5)
        throw Error("Report did not close")
}

; Battle watches an available battle through the VCR's controls and Done button.
Battle() {
    if !InStr(ControlGetText(Integer(ReadState()["messageButton.1"])), "View")
        MessageGoto()
    MessageGoto()
    hwnd := WaitDialog("Battle VCR")
    ClickId(hwnd, Controls["IDC_VCR_FWD_ALL"])
    ClickId(hwnd, Controls["IDOK"])
    if !WinWaitClose(hwnd, , 5)
        throw Error("Battle playback did not close")
}

; ResearchDetails opens the dialog and inspects an expected benefit tooltip.
ResearchDetails() {
    if !Integer(ReadState()["hwnd.research"])
        Keys("{F5}")
    hwnd := WaitDialog("Research")
    state := WaitFor((s) => s.Has("researchFuture"), "expected research benefits")
    parts := StrSplit(state["researchFuture"], "|")
    ClickPoint(hwnd, Integer(parts[2]) // 2, Integer(parts[3]) + Integer(state["fontHeight"]) // 2)
}

; ResearchBudget uses the research arrows to set a bounded percentage.
ResearchBudget(percent) {
    hwnd := WaitDialog("Research")
    Loop 100 {
        current := Integer(ReadState()["researchBudget"])
        if current = percent {
            ClickId(hwnd, Controls["IDCANCEL"])
            return
        }
        ClickRect(current < percent ? "researchUp.0" : "researchDown.0")
    }
    throw Error("Research budget did not reach " percent)
}

; SummaryDetail clicks the summary field's runtime font-based geometry.
SummaryDetail(field) {
    state := ReadState(), hwnd := Integer(state["hwnd.summary"]), font := Integer(state["fontHeight"])
    WinGetClientPos , , &width, &height, hwnd
    if field = "fleet"
        ClickPoint(hwnd, 30, 2 * font + 25, , "Right")
    else {
        row := ((height - (2 * font - 4) - 4 * font - 2) // 6 + 1) & 0xFFFE
        y := 4 * font - 4 + (field = "radiation" ? 2 : 0) * row + row // 2
        ClickPoint(hwnd, 30, y)
    }
}

; MergeFleet merges selected fleets using the normal multiple-selection list.
MergeFleet(target) {
    ClickControl(ReadState()["button.10"])
    hwnd := WaitDialog("Merge Fleets")
    list := DllCall("GetDlgItem", "ptr", hwnd, "int", Controls["IDC_MERGE_FLEETS_LIST"], "ptr")
    Choose(list, target)
    ClickId(hwnd, Controls["IDOK"])
    WinWaitClose hwnd, , 5
}

; ProductionAt adds an order after selecting the named planet.
ProductionAt(planet, item, quantity, leftover := -1, position := -1) {
    state := ReadState()
    if Integer(state["hwnd.production"]) {
        selected := StrSplit(state["selection"], "|")
        if selected[1] != "1" || StrSplit(state["planet." selected[2]], "|")[1] != planet
            throw Error("A different planet's production queue is already open")
        Production(item, quantity, leftover, position)
        return
    }
    PlanetSelect(planet)
    Production(item, quantity, leftover, position)
}

; CloseBrowser dismisses the technology browser's Done control.
CloseBrowser() {
    hwnd := Integer(ReadState()["hwnd.browser"])
    ClickId(hwnd, Controls["IDCANCEL"])
    WaitFor((s) => s["hwnd.browser"] = "0" && !DllCall("IsWindowVisible", "ptr", hwnd), "technology browser closed")
}

; DefaultProductionTemplate imports the current automatic queue as the default.
DefaultProductionTemplate(terraform) {
    if terraform
        Production("Min Terraform (Auto Build)", 2, 1, 0)
    if !Integer(ReadState()["hwnd.production"])
        Keys("q")
    productionWindow := Integer(WaitFor((s) => Integer(s["hwnd.production"]) != 0, "production queue")["hwnd.production"])
    ClickRect("productionDiamond.0", , "Right")
    PopupChoice("<Customize>")
    template := WaitDialog("Customize Production Templates")
    ClickId(template, Controls["IDC_ZIP_PROD_PRESET_1"])
    ClickId(template, Controls["IDC_IMPORT"])
    ClickId(template, Controls["IDOK"])
    ClickId(productionWindow, Controls["IDOK"])
    WinWaitClose productionWindow, , 5
}

; SaveTransportTemplate imports the current colonist order into a named preset.
SaveTransportTemplate() {
    ClickRect("reference.5", , "Right")
    PopupChoice("<Customize>")
    template := WaitDialog("Customize Zip Orders")
    ClickId(template, Controls["IDC_ZIP_PROD_PRESET_1"])
    ClickId(template, Controls["IDC_IMPORT"])
    rename := WaitDialog("Rename")
    ControlSetText "DropCol", DllCall("GetDlgItem", "ptr", rename, "int", Controls["IDC_EDIT1"], "ptr")
    ClickId(rename, Controls["IDOK"])
    ClickId(template, Controls["IDOK"])
    WinWaitClose template, , 5
}

; ProductionTileDetail inspects a production item through its normal popup.
ProductionTileDetail() {
    hwnd := Integer(ReadState()["hwnd.planetQueue"])
    SelectIndex(hwnd, 1)
    ControlClick hwnd, , , "Right"
    Sleep 50
}

; EditSantaMaria selects the existing colony ship and enters edit mode.
EditSantaMaria() {
    hwnd := OpenDesigner()
    ClickId(hwnd, Controls["IDC_DESIGNER_EXISTING"])
    Choose(DllCall("GetDlgItem", "ptr", hwnd, "int", Controls["IDC_COMBOBOX"], "ptr"), "Santa Maria")
    ClickId(hwnd, Controls["IDC_EDIT"])
}

; ReplaceEngine removes the old engine through drag and fits its replacement.
ReplaceEngine() {
    hwnd := Integer(ReadState()["hwnd.designer"])
    rect := StrSplit(ReadState()["slot.0"], "|")
    from := ClientPoint(hwnd, (Integer(rect[2]) + Integer(rect[4])) // 2, (Integer(rect[3]) + Integer(rect[5])) // 2)
    list := DllCall("GetDlgItem", "ptr", hwnd, "int", Controls["IDC_DESIGNER_COMPONENT_LIST"], "ptr")
    to := ClientPoint(list, 20, 20)
    WinActivate hwnd
    MouseClickDrag "Left", from[1], from[2], to[1], to[2], 2
    Sleep 50
    AddPart("Engines", "Daddy Long Legs 7", 0)
}

; BuildDestroyer fits the engine, three phasers and two armor plates.
BuildDestroyer() {
    AddPart("Engines", "Radiating Hydro-Ram Scoop", 0)
    Loop 3
        AddPart("Beam Weapons", "Yakimora Light Phaser", A_Index)
    AddPart("Armor", "Carbonic Armor", 4, 2)
}

; TransferShip moves one ship into the newly split fleet through its arrow.
TransferShip() {
    hwnd := WaitDialog("Transfer")
    state := WaitFor((s) => s.Has("transferMode"), "ship transfer layout")
    for key, value in state {
        if SubStr(key, 1, 15) != "transferButton."
            continue
        button := StrSplit(value, "|")
        if button[2] = "1" && button[3] = "1" && button[4] = "0" {
            ClickPoint(hwnd, (Integer(button[5]) + Integer(button[7])) // 2, (Integer(button[6]) + Integer(button[8])) // 2)
            ClickId(hwnd, Controls["IDOK"])
            WinWaitClose hwnd, , 5
            return
        }
    }
    throw Error("No ship transfer arrow is available")
}

; TransferFuel adjusts the real gauge and arrow controls to an exact fuel amount.
TransferFuel(amount) {
    ClickRect("reference.4")
    hwnd := WaitDialog("Cargo Transfer")
    state := WaitFor((s) => s.Has("transferFleet.0"), "fuel transfer layout")
    capacity := Integer(StrSplit(state["transferFleet.0"], "|")[4])
    for key, value in state {
        if SubStr(key, 1, 15) != "transferButton."
            continue
        gauge := StrSplit(value, "|")
        if gauge[1] = "4" && gauge[2] = "0" && gauge[3] = "0" {
            from := ClientPoint(hwnd, Integer(gauge[5]) + 2, (Integer(gauge[6]) + Integer(gauge[8])) // 2)
            to := ClientPoint(hwnd, Integer(gauge[5]) + (Integer(gauge[7]) - Integer(gauge[5]) - 2) * amount // capacity, (Integer(gauge[6]) + Integer(gauge[8])) // 2)
            WinActivate hwnd
            MouseClickDrag "Left", from[1], from[2], to[1], to[2], 2
            Sleep 50
            break
        }
    }
    Loop 600 {
        state := ReadState(), current := Integer(StrSplit(state["transferFleet.0"], "|")[3])
        if current = amount {
            ClickId(hwnd, Controls["IDOK"])
            WinWaitClose hwnd, , 5
            return
        }
        found := false
        for key, value in state {
            if SubStr(key, 1, 15) != "transferButton."
                continue
            button := StrSplit(value, "|")
            if button[1] = "4" && Integer(button[2]) = (current < amount ? 0 : 1) && button[3] = "1" {
                ClickPoint(hwnd, (Integer(button[5]) + Integer(button[7])) // 2, (Integer(button[6]) + Integer(button[8])) // 2)
                found := true
                break
            }
        }
        if !found
            throw Error("No fuel adjustment arrow is available")
    }
    throw Error("Fuel transfer did not reach " amount)
}

; SetWarp clicks the selected leg's warp gauge at the requested speed.
SetWarp(speed) {
    rect := StrSplit(ReadState()["reference.0"], "|")
    ClickPoint(rect[1], Integer(rect[2]) + (Integer(rect[4]) - Integer(rect[2]) - 2) * speed // 11, (Integer(rect[3]) + Integer(rect[5])) // 2)
}

; LateMessages performs late tutorial message milestones and score inspection.
LateMessages(bold) {
    switch bold {
        case 496: ReadTo(3)
        case 555: ReadTo(6), FleetSelect(12)
        case 567: PlanetSelect("Stove Top"), PlanetClick("Hacker", "Ctrl")
        case 569: ReadTo(17)
        case 608: ReadTo(10), PlanetSelect("Stove Top")
        case 636:
            ReadMessages()
            Keys("{F10}")
            Sleep 50
            score := WaitFor((s) => Integer(s["hwnd.score"]) != 0, "final score report")
            ClickId(Integer(score["hwnd.score"]), Controls["IDCANCEL"])
        case 606, 619:
            ReadMessages()
            WaitFor((s) => s["turnDone"] = "1", "late tutorial turn completion")
            Generate()
        default: ReadMessages()
    }
}

; SelectIndex selects a list row with one click, avoiding ListBox double-click actions.
SelectIndex(control, index) {
    control := Integer(control)
    if WinGetClass(control) != "ListBox" {
        ControlChooseIndex index, control
        Sleep 50
        return
    }
    SendMessage 0x197, index - 1, 0, , control
    rect := Buffer(16)
    if SendMessage(0x198, index - 1, rect.Ptr, , control) = -1
        throw Error("Cannot locate list row " index)
    ClickPoint(control, (NumGet(rect, 0, "int") + NumGet(rect, 8, "int")) // 2, (NumGet(rect, 4, "int") + NumGet(rect, 12, "int")) // 2)
}

; DismissPopup releases an owned native menu left open by a failed action.
DismissPopup() {
    global GamePid
    for popupHwnd in WinGetList("ahk_class #32768 ahk_pid " GamePid) {
        if DllCall("IsWindowVisible", "ptr", popupHwnd) {
            SendEvent "{Esc}"
            Sleep 50
            return
        }
    }
}

; ConfigureWindows positions the game and tutor on separate desktop surfaces.
ConfigureWindows(state) {
    WinMove 0, 0, 1280, 960, Integer(state["hwnd.frame"])
    WinMove 1282, 20, , , Integer(state["hwnd.tutor"])
    Sleep 100
}
