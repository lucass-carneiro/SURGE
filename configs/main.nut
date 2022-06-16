function surge::quit_called() {
    print("Quit event received.");
}

function surge::mouse_button_down(which, button, state, clicks, x, y) {
    print("---------------------");
    print("Mouse button pressed.");
    print("which = " + which);
    print("button = " + button);
    print("state = " + state);
    print("clicks = " + clicks);
    print("x = " + x);
    print("y = " + y);
    print("---------------------");
}

function surge::mouse_button_up(which, button, state, clicks, x, y) {
    print("---------------------");
    print("Mouse button released.");
    print("which = " + which);
    print("button = " + button);
    print("state = " + state);
    print("clicks = " + clicks);
    print("x = " + x);
    print("y = " + y);
    print("---------------------");
}

function surge::draw() {
    // Draws a colored square on the screen
    surge.renderer.draw_line(300, 300, 400, 300, 255, 0, 0, 255);
    surge.renderer.draw_line(400, 300, 400, 400, 0, 255, 0, 255);
    surge.renderer.draw_line(400, 400, 300, 400, 0, 0, 255, 255);
    surge.renderer.draw_line(300, 400, 300, 300, 255, 255, 255, 255);
}
