/*
 * webservdata.h
 *
 *  Created on: Jun 12, 2017
 *      Author: Kevin
 */

#ifndef WEBSERVDATA_H_
#define WEBSERVDATA_H_

const char Header[] =
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/materialize/0.98.2/css/materialize.min.css\">"
"<link href=\"https://fonts.googleapis.com/icon?family=Material+Icons\" rel=\"stylesheet\">"
"<nav><div class=\"nav-wrapper teal\">"
"<a href=\"\" class=\"brand-logo\">Irrigation System</a>"
"<ul id=\"nav-mobile\" class=\"right hide-on-med-and-down\"></ul>"
"</div></nav>"
"<div class=\"card teal lighten-2\">"
"<div class=\"card-content\">"
"<span class=\"card-title\">System Info:</span>"
"<p>Current System Information</p>"
"</div>"
"<ul class=\"collection\">"
"<li class=\"collection-item avatar\">"
"<i class=\"material-icons circle blue\">import_export</i>"
"<a class=\"collection-item\"><span class=\"badge\">"
;

const char Header2[] =
"</span>MQTT</a>"
"</li>"
"<li class=\"collection-item avatar\">"
"<i class=\"material-icons circle blue\">developer_board</i>"
"<a href=\"http://192.168.0.110:1880\" class=\"collection-item\"><span class=\"badge\">"
;


const char Header3[] =
"</span>NodeRed</a>"
"</li>"
"<li class=\"collection-item avatar\">"
"<i class=\"material-icons circle blue\">blur_on</i>"
"<a class=\"collection-item\"><span class=\"badge\">"
;

const char Header4[] =
"</span>Zone 1</a>"
"</li>"
"<li class=\"collection-item avatar\">"
"<i class=\"material-icons circle blue\">blur_on</i>"
"<a class=\"collection-item\"><span class=\"badge\">"
;

const char Header5[] =
"</span>Zone 2</a>"
"</li>"
"<li class=\"collection-item avatar\">"
"<i class=\"material-icons circle blue\">blur_on</i>"
"<a class=\"collection-item\"><span class=\"badge\">"
;

const char Header6[] =
"</span>Zone 3</a>"
"</li>"
"<li class=\"collection-item avatar\">"
"<i class=\"material-icons circle blue\">blur_on</i>"
"<a class=\"collection-item\"><span class=\"badge\">"
;

const char Body1[] =
"</span>Zone 4</a>"
"</li>"
"</ul>"
"</div>"
"<div class=\"card teal lighten-2\">"
"<div class=\"card-content\">"
"<span class=\"card-title\">MQTT Topics</span>"
"<p>Current Topics being subscribed and published to</p>"
"</div>"
"<ul class=\"collection\">"
"<li class=\"collection-item avatar\">"
"<i class=\"material-icons circle blue\">file_download</i>"
"<table class=\"striped\">"
"<thead>"
"<tr><th>Subscriptions</th></tr>"
"</thead>"
"<tbody>"
"<tr><td>"
;

const char Table1[] =
"</td></tr>"
"<tr><td>"
;

const char Body2[] =
"</td></tr>"
"</tbody>"
"</table>"
"</li>"
"<li class=\"collection-item avatar\">"
"<i class=\"material-icons circle blue\">file_upload</i>"
"<table class=\"striped\">"
"<thead>"
"<tr><th>Publications</th></tr>"
"</thead>"
"<tbody>"
"<tr><td>"
;

const char Body3[] =
"</td></tr>"
"</tbody>"
"</table>"
"</li>"
"</ul>"
"</div>"
;

const char Form[] =
"<div class=\"card cyan lighten-2\">"
"<div class=\"card-content\">"
"<span class=\"card-title\">Set Values:</span>"
"<p>Manually set relays</p>"
"</div>"

"<form action=\"\" method=\"post\">"
"<ul class=\"collection\">"

"<li class=\"collection-item\">"
"Relay 1 - Zone1<BR>"
"<input id=\"Relay1\" name=\"Relay1\" type=\"radio\" class=\"element radio\" value=\"1\" />"
"<label for=\"Relay1\">On</label>"
"<input id=\"Relay1x\" name=\"Relay1\" type=\"radio\" class=\"element radio\" value=\"0\" />"
"<label for=\"Relay1x\">Off</label>"
"</li>"

"<li class=\"collection-item\">"
"Relay 2 - Zone2<BR>"
"<input id=\"Relay2\" name=\"Relay2\" type=\"radio\" class=\"element radio\" value=\"1\" />"
"<label for=\"Relay2\">On</label>"
"<input id=\"Relay2x\" name=\"Relay2\" type=\"radio\" class=\"element radio\" value=\"0\" />"
"<label for=\"Relay2x\">Off</label>"
"</li>"

"<li class=\"collection-item\">"
"Relay 3 - Zone3<BR>"
"<input id=\"Relay3\" name=\"Relay3\" type=\"radio\" class=\"element radio\" value=\"1\" />"
"<label for=\"Relay3\">On</label>"
"<input id=\"Relay3x\" name=\"Relay3\" type=\"radio\" class=\"element radio\" value=\"0\" />"
"<label for=\"Relay3x\">Off</label>"
"</li>"

"<li class=\"collection-item\">"
"Relay 4 - Zone4<BR>"
"<input id=\"Relay4\" name=\"Relay4\" type=\"radio\" class=\"element radio\" value=\"1\" />"
"<label for=\"Relay4\">On</label>"
"<input id=\"Relay4x\" name=\"Relay4\" type=\"radio\" class=\"element radio\" value=\"0\" />"
"<label for=\"Relay4x\">Off</label>"
"</li>"

"<li class=\"collection-item\">"
"<button class=\"btn waves-effect waves-light\" type=\"submit\" name=\"action\">Submit"
"<i class=\"material-icons left\">send</i>"
"</button>"
"<button class=\"btn waves-effect waves-light\" type=\"reset\" name=\"action\">Reset"
"<i class=\"material-icons left\">history</i>"
"</button>"
"</li>"
"</ul>"
"</form>"

"</div>"
;






#endif /* WEBSERVDATA_H_ */
