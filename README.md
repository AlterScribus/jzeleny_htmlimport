# jzeleny_htmlimport
# Refactoring of the HTML import plugin for better styles use

- Original repo : 'public' branch of `git://fedorapeople.org/~jzeleny/scribus.git` 
- Master mantis URL : http://bugs.scribus.net/view.php?id=11060

Current HTML plugin has several shortcomings related to a limited support of HTML tags. Attached archive contains new implementation of HTML import plugins which adds various new features.

First of all the plugin adds a support for basic set of CSS rules. The second main advantage is that the plugin can be now very easily extended to support new HTML tags via the CSS definition. Also it is quite easy to modify the CSS definition of extended plugins.

A complete list of supported features is on the mailing list:
http://lists.scribus.net/pipermail/scribus-dev/2012-August/001669.html 

### State of the dev :
- on the architecture side, it is a definite improvement
- currently the importer creates to much styles and basic styles (h1...h6, etc...) are not clearly defined after import in style manager (see details and discussion in mantis)

### Technical explanations

The plugin has been designed to be fully compatible with
current behavior. I can check again to make sure it still is. Do you
perhaps have a test set of HTML code that would verify that?

That means everything that was supported should be still supported,
maybe with minor visual nuances that can be in most cases easily corrected.

The main advantage now is flexibility. The plugin can be now very easily
extended to support more tags or more features as long as they are
supported by gettext and HTML specification.

For user, the biggest difference is a support for HTML attribute
"style". In that attribute you can define various styles that are valid
CSS and the plugin will recognize them.  The definition is the same as
in standard CSS:

attribute: value[; attribute: value[ ... ]]

Later definition overwrites all previous ones. Some attributes have
pre-defined visual appearance, that can be changed by defining
additional style modifications.

List of supported tags:
`ul, ol, li, pre, br, a, sup, sub, img, center, p, pre, div, h1 - h6, code, em, i, strong, b, u , ins, del`

List of supported CSS attributes and allowed values:
- font-weight: bold/normal
- font-style: italic/normal
- font-family: <font_string>
- color: <color_string>
- text-align: left, right, center, justify
- text-decoration: underline, strikethrough, none```

Font string can be defined as arbitrary string representation of font
loaded by Scribus. I tested both family name (e.g. `DejaVu Sans`) and full
font name (e.g. `DejaVu Sans Bold`), both should work as far as the font
is loaded in Scribus.

Color string was kinda blind shooting on my part. Color names defined by
the document are working and IIRC hex codes are working as well. I think
I even saw a definition like `rgb(255,0,0)` to go through and work. If you
can verify all this, that'd be helpful, I don't have much time left
these days. 

One thought: I think that in case of hexa and rgb
definition, the color integrity might be compromised since I don't
perform any conversion based on color model. I have no idea whether it's
done on lower levels. To give you an idea what am I doning in the code,
I just call `font->setColor(value);` that's it.

`margin-top, margin-bottom, margin-left, font-size` : these can have size values defined either as fixed size or relative size
- XX px - fixed size in points
- XX pt - fixed size in points
- XX % - relative size in percentage
- XX em - relative size, similar to percentage
- +XX px/pt/%/em - absolute/relative size respectively, added to the current size (i.e. +50px -> use current size plus 50 more points)```

Code-wise, the approach has been changed completely. The visual
appearance for every element is now stored only in s style string. These
strings are then parsed and stored in form of stack of `gtParagraphStyle`
structures which are then pushed/popped as HTML parsing goes on. I hope
this description is sufficient.

Additional notes : 
- I have a bunch of fixes in gettext, one extension of Python
scripting interface and then I have some hack-ish solutions for some
problems I've been having with automated scribus launching. I will send
you everything probably during this week. Some patches will be good to
go, some will be just to outline what problems I've been having, I'm
certain there are better solutions for them.
- Also I started to do some work on rewriting scribus core to support
headless run but then I got stuck with the HTML plugin so I will need to
get back on track with that once I have some time. Hope that will be soon.
"
