// JavaScript utilities used on multiple pages
function makeElement(what, cssclass, elt) {
  var e = document.createElement(what);
  if (cssclass)
    e.setAttribute('class', cssclass);
  if (elt)
    elt.appendChild(e);
  return e;
}
function IMG(cssclass, elt) { return makeElement('IMG', cssclass, elt); }
function DIV(cssclass, elt) { return makeElement('DIV', cssclass, elt); }
function SPAN(cssclass, elt) { return makeElement('SPAN', cssclass, elt); }
function A(cssclass, elt) { return makeElement('A', cssclass, elt); }
function H1(cssclass, elt) { return makeElement('H1', cssclass, elt); }
function H2(cssclass, elt) { return makeElement('H2', cssclass, elt); }
function H3(cssclass, elt) { return makeElement('H3', cssclass, elt); }
function LI(cssclass, elt) { return makeElement('LI', cssclass, elt); }
function UL(cssclass, elt) { return makeElement('UL', cssclass, elt); }
function TABLE(cssclass, elt) { return makeElement('TABLE', cssclass, elt); }
function TR(cssclass, elt) { return makeElement('TR', cssclass, elt); }
function TD(cssclass, elt) { return makeElement('TD', cssclass, elt); }
function TEXT(contents, elt) {
  var e = document.createTextNode(contents);
  if (elt)
    elt.appendChild(e);
  return e;
}

