# jzeleny_htmlimport
refactoring of the HTML import plugin for better styles use

- Original repo : git://fedorapeople.org/~jzeleny/scribus.git
- Master mantis URL : http://bugs.scribus.net/view.php?id=11060

Current HTML plugin has several shortcomings related to a limited support of HTML tags. Attached archive contains new implementation of HTML import plugins which adds various new features.

First of all the plugin adds a support for basic set of CSS rules. The second main advantage is that the plugin can be now very easily extended to support new HTML tags via the CSS definition. Also it is quite easy to modify the CSS definition of extended plugins.

A complete list of supported features is on the mailing list:
http://lists.scribus.net/pipermail/scribus-dev/2012-August/001669.html 

State of the dev :
- on the architecture side, it is a definite improvement
- currently the importer creates to much styles and basic styles (h1...h6, etc...) are not clearly defined after import in style manager (see details and discussion in mantis)
