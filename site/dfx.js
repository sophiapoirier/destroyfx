// JavaScript utilities used on multiple pages
function makeElement(what, cssclass, elt) {
  let e = document.createElement(what);
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
  let e = document.createTextNode(contents);
  if (elt)
    elt.appendChild(e);
  return e;
}

function FullImage(f) {
  let olay = DIV('', document.body);
  olay.style.position = 'fixed';
  olay.style.backgroundColor = 'rgba(0,0,0,.75)';
  olay.style.backdropFilter = 'blur(8px)';
  olay.style.width = '100%';
  olay.style.height = '100%';
  olay.style.zIndex = 999;
  olay.style.top = 0;
  olay.style.left = 0;
  olay.style.cursor = 'pointer';
  
  let img = IMG('', olay);
  img.src = f;
  img.style.position = 'absolute';
  img.style.margin = 'auto';
  img.style.top = 0;
  img.style.right = 0;
  img.style.bottom = 0;
  img.style.left = 0;
  img.style.border = '2px solid #000';
  img.style.boxShadow = '5px 5px 32px 16px rgba(0,0,0,.25)';

  window.location = '#img';
  
  let clear = () => {
    if (olay)
      olay.parentNode.removeChild(olay);
    olay = null;
  };
  
  window.onpopstate = clear;
  
  olay.onclick = (evt) => {
    evt.stopPropagation();
    history.back();
    // Redundant, but in case history is not working
    // like we expect.
    clear();
  };
}
