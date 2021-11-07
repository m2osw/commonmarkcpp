
# Introduction

The commonmarkcpp library is used to convert markdown files (.md) into HTML.
It is based on the [commonmark specifications](https://spec.commonmark.org/).


# Errors in Specifications

## Link Destination

The link destination is defined as follow:

> A link destination consists of either
> 
> * a sequence of zero or more characters between an opening < and a closing
>   \> that contains no line endings or unescaped < or > characters, or
> 
> * a nonempty sequence of characters that does not start with <, does not
>   include ASCII control characters or space character, and includes
>   parentheses only if (a) they are backslash-escaped or (b) they are part
>   of a balanced pair of unescaped parentheses. (Implementations may impose
>   limits on parentheses nesting to avoid performance issues, but at least
>   three levels of nesting should be supported.)

When we know that the `<uri>` must also be written between parenthesis as
in `(<uri>)`. Otherwise, it is a separate link appearing after the
preceeding short reference.

## Auto Link

In the following sentence "other" should be followe by "than":

> An absolute URI, for these purposes, consists of a scheme followed by
> a colon (:) followed by zero or more characters other ASCII control
> characters, space, <, and >. If the URI includes these characters,
> they must be percent-encoded (e.g. %20 for a space).

Also "space" could be plural.

## Setext (Section 4.3)

The Setext and Example 46 are contradictory. The `"--"` is cleared added
to the paragraph and therefore is clearly viewed as a valid paragraph
entry. However, the example is then adding `"**"` which can be viewed
as an Setext marker and yet the example outputs it as a normal paragraph
entry.


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/commonmarkcpp/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
