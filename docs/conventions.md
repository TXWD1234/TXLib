# **TXLib Conventions**
The convention specifications of everything in TXLib.

# Github Commits
## Prefix
All commits should have prefix indicating which module / which component that commit is modifying.
The format of prefix is:
```
<module>: <component>:
```
For example, a refactor of data structure `tx::BinarySetView` in TXData should have commit message title prefix like below:
```
TXData: BinarySetView:
```
After the prefix, the brief explaination of the content of this commit is followed.
If the brief explaination isn't enough, detailed change log can be added as body of the commit.

For the `<component>` title, if the workis related to a specific identifier in the code, for example a namespace or a class name, `<component>` could be that identifier directly. In another case where it is a specific file such as `type_traits.hpp`, `<component>` can just be the file name.

If the commit changes multiple components (within the same module), the `<component>` part of the prefix can be omitted.
If the commit changes multiple modules, it is considered as a global refactor, and the `<module>` part of the prefix can be omitted.
if the commit changes root infrastructures such as `.clang-format`, use `TXLib:` prefix.

## Cases and Symbols
### Cases
For PR, the title share the same rule as **titles** in English.
For normal commits, the title share the same rule as **sentences** in English but without period at the end.

### `[n]` Symbol
In occasions when a process takes multiple commits, the `[n]` symbol can be used to indicate the index of commits. Where `n` is the number of the current commit.
The `[n]` symbol should be placed after the header, before the ":".
Format:
```
<module>: <component> [n]:
```
For example:
```
TXData: BinarySetView [1]: Refactor ...
TXData: BinarySetView [2]: Refactor ...
TXData: BinarySetView [3]: Refactor ...
```

## PR

# Code
## Naming
| Field | Specification |
| - | - |
| Variable     | camelCase  |
| Function     | camelCase  |
| Class / Type | PascalCase |
| Concept      | snake_case |

### Special Case
- Anything about Type Traits are in snake_case.
- Basic math types such as `u32` and `vec2` are in snake_case, as long as there's only one word. (so the `_` is never used)
- Std addons follows whatever naming convensions that Std compoenent has.

### Rules
- Class members all have `m_` prefix.
- Class implementation functions and classes all have `_impl` suffix.