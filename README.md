## WingCodeEdit

A Lightweight Qt-based code and text editor widget for Qt6. This widget comes from [qtextpad](https://github.com/zrax/qtextpad) and I make some improvements such as newer intergrated syntax highlighting which powers [Kate](https://kate-editor.org/), code completing and symbol mark.

> This project is used for scripting with [WingHexExplorer2](https://github.com/Wing-summer/WingHexExplorer2) in version 2.2.1 newer or later.

## Features

* Syntax highlighting support for hundreds of file types and dialects.
* Fast, lightweight design -- no plugins, session management, etc.
  to deal with when you just want to open a file.
* Attractive design with Light or Dark Theme.
* Many helpful code editing features.
* Code completion is supported.
* Symbol mark that can be used to display bookmarks or breakpoints.
* Model to show squiggle information.
* A lite widget to show tooltip signature.

## Custom indentation

`setIndentationStrategy()` installs an optional callback owned by the editor.
`autoIndent()` remains the master switch. Passing `{}` clears the callback.
Without a callback the editor keeps its existing behavior. A `std::nullopt`
result inherits indentation for newlines and leaves current lines unchanged.
Returning an empty `QString` explicitly requests no indentation.
Results containing anything other than spaces and tabs
are ignored (using the same behavior as `std::nullopt`).

The callback handles both triggers through `IndentationContext::trigger`:
`IndentationTrigger::NewLine` runs before newline insertion with a cursor at
selectionStart(); `IndentationTrigger::TextInput` runs after typing, or when
`reindentCurrentLine()` is called after completion. The context also provides
the editor and current block's code start offset (for non-code prefixes).
It is not called inside a protected prefix or a protected block.
Do not modify the document from the callback.
The editor applies the result together with the newline in one undo operation.

```cpp
editor->setIndentationStrategy(
    [](const WingCodeEdit::IndentationContext &context)
        -> std::optional<QString> {
        // Select a host-provided language policy using
        // context.editor.highlighter()->definition().indenter().
        // Include wingsyntaxhighlighter.h and KSyntaxHighlighting/Definition
        // when accessing the syntax definition.
        return std::nullopt;
    });
```

Language policies (Python, Lua, etc.) belong to the host. No changes to
syntax-highlighting are needed.

For `TextInput`, the context points to the cursor after insertion. A result
replaces only leading spaces/tabs after `codeStart`; `std::nullopt` leaves the
line unchanged. Protected
prefixes, read-only mode and the auto-indent switch are respected. Typing and its
indentation adjustment share one undo operation. Completion handlers should call
`reindentCurrentLine()` after inserting their text, inside the same edit block.

## License

This project uses the GPL version 3.
