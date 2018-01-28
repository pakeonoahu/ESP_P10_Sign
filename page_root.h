#ifndef PAGE_ROOT_H
#define PAGE_ROOT_H

const char PAGE_ROOT[] PROGMEM = R"=====(
<strong>ESPSign</strong> <hr> <a href="config/net.html" style="width:250px" class="btn btn--m btn--blue">Network Configuration</a><br> <a href="admin.html" style="width:250px" class="btn btn--m btn--blue">Administration</a><br> <a href="status/net.html" style="width:250px" class="btn btn--m btn--blue">Network Status</a><br> <a href="/sign" style="width:250px" class="btn btn--m btn--blue">Sign</a><br><a href="/update" style="width:250px" class="btn btn--m btn--blue">Firmware update</a><hr> <pre id="name"></pre> <script>setValues("/rootvals");</script>
)=====";

void send_root_vals() {
    String values = "";
    values += "name|div|" + (String)config.name + "\n";
    values += "title|div|" + String("ESPS - ") + (String)config.name + "\n";
    web.send(200, PTYPE_PLAIN, values);
}
#endif
