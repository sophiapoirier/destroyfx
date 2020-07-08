function openWindow(theURL, windowName, wide, tall)
{
	var x = (screen.width - wide) / 2;
	var y = (screen.height - tall) / 2;
	var newWindow = window.open(theURL, windowName, 'width='+wide+',height='+tall+',scrollbars=no,resizable=no,top='+y+',left='+x+',screenX='+x+',screenY='+y);
	if (newWindow)
		return false;
	else
		return true;
}

function openPicture_good(pictureURL, pictureName, wide, tall)
{
	var x = (screen.width - wide) / 2;
	var y = (screen.height - tall) / 2;
	var taller = tall - (-5);

	var pictureWindow = window.open("", pictureName, 'resizable=no,location=no,menubar=no,status=no,scrollbars=no,toolbar=no,width='+wide+',height='+taller+',screenX='+x+',screenY='+y+',top='+y+',left='+x);
	if (!pictureWindow)
		return true;
	pictureWindow.document.open();
	pictureWindow.document.writeln('<?xml version="1.0" encoding="ISO-8859-1"?>');
	pictureWindow.document.writeln('<?xml-stylesheet href="#internalstyle" type="text/css"?>');
	pictureWindow.document.writeln('<!DOCTYPE html \n\tPUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" \n\t"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">');
	pictureWindow.document.writeln('<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">');
	pictureWindow.document.writeln('<head>');
	pictureWindow.document.writeln('\t<title>'+pictureName+'</title>');
	pictureWindow.document.writeln('\t<style type="text/css" id="internalstyle">');
	pictureWindow.document.writeln('\t\tbody { background-color: black ; margin: 0px ; padding: 0px }');
	pictureWindow.document.writeln('\t\timg { margin: 0px ; padding: 0px }');
	pictureWindow.document.writeln('\t</style>');
	pictureWindow.document.writeln('</head>');
	pictureWindow.document.writeln('<body>');
	pictureWindow.document.writeln('\t<div>');
	pictureWindow.document.writeln('\t\t<img src="'+pictureURL+'" width="'+wide+'" height="'+tall+'" alt="'+pictureName+'" />');
	pictureWindow.document.writeln('\t</div>');
	pictureWindow.document.writeln('</body>');
	pictureWindow.document.writeln('</html>');
	pictureWindow.document.close();
	return false;
}

function openPicture(pictureURL, pictureName, wide, tall)
{
	var border = 7;
	var x = ((screen.width - wide) / 2) - border;
	var y = ((screen.height - tall) / 2) - border;
	var wider = wide - (-border * 2);
	var taller = tall - (-border * 2);

	var pictureWindow = window.open("", pictureName, 'resizable=no,location=no,menubar=no,status=no,scrollbars=no,toolbar=no,width='+wider+',height='+taller+',screenX='+x+',screenY='+y+',top='+y+',left='+x);
	if (!pictureWindow)
		return true;
	pictureWindow.document.open();
	pictureWindow.document.writeln('<?xml version="1.0" encoding="ISO-8859-1"?>');
	pictureWindow.document.writeln('<?xml-stylesheet href="#internalstyle" type="text/css"?>');
	pictureWindow.document.writeln('<!DOCTYPE html \n\tPUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" \n\t"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">');
	pictureWindow.document.writeln('<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">');
	pictureWindow.document.writeln('<head>');
	pictureWindow.document.writeln('\t<title>'+pictureName+'</title>');
	pictureWindow.document.writeln('\t<style type="text/css" id="internalstyle">');
	pictureWindow.document.writeln('\t\tbody { background-color: black ; margin-left: '+border+'px ; margin-top: '+border+'px ; margin-right: 0px ; margin-bottom: 0px ; padding: 0px }');
	pictureWindow.document.writeln('\t\timg { margin: 0px ; padding: 0px }');
	pictureWindow.document.writeln('\t</style>');
	pictureWindow.document.writeln('</head>');
	pictureWindow.document.writeln('<body>');
	pictureWindow.document.writeln('\t<div>');
	pictureWindow.document.writeln('\t\t<img src="'+pictureURL+'" width="'+wide+'" height="'+tall+'" alt="'+pictureName+'" />');
	pictureWindow.document.writeln('\t</div>');
	pictureWindow.document.writeln('</body>');
	pictureWindow.document.writeln('</html>');
	pictureWindow.document.close();
	return false;
}
