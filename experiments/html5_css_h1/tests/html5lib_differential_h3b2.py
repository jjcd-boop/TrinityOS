#!/usr/bin/env python3
"""Compare standalone Trinity H3B2 body tree to html5lib (host test oracle).

html5lib is a TEST-ONLY Python dependency; no external browser source or
Python runtime is embedded in Trinity.
"""
import subprocess
import sys
import html5lib

def expected_body_children(source):
    document = html5lib.parse(source, treebuilder="etree", namespaceHTMLElements=False)
    body = document.find("body")
    if body is None:
        raise RuntimeError("No body from reference parser")
    out = []
    def escaped(value):
        # Match the C++ tree-dumper's escaped, line-delimited text encoding.
        return (value.replace("\\", "\\\\").replace("\n", "\\n")
                     .replace("\r", "\\r").replace("\t", "\\t"))
    def children(element):
        if element.text:
            out.append("T:" + escaped(element.text))
        for node in element:
            # Comments aren't in the intentionally limited element/text
            # differential, but their adjacent text/tail still matters.
            if isinstance(node.tag, str):
                out.append("E:" + str(node.tag))
                children(node)
                out.append("/E")
            if node.tail:
                out.append("T:" + escaped(node.tail))
    children(body)
    return out

cases = [
    ("basic misnested", "<b><i>1</b>2</i>", True),
    ("direct furthest block", "<b>1<p>2</b>3</p>", True),
    ("nested link", "<p><a href=x>A<a href=y>B</a>C</p>", True),
    ("intermediate italic", "<b><i><p>X</b>Y", True),
    ("intermediate closing", "<b><i><p>X</b>Y</i>Z</p>", True),
    ("multiple intermediate", "<b><em><i><div>X</div></b>Y</em>Z</i>", True),
    ("formatting attribute", "<em class=x><i><div>A</em>B</i></div>", True),
    ("nested strong emphasis", "<strong><em><p>A</strong>B</em>C</p>", True),
    ("formatting inside nonformat span", "<b><span><p>A</b>B</span>C", True),
    ("three formatting intermediates", "<b><i><em><p>A</b>B</em>C</i>", True),
    ("four intermediate formatting tags", "<b><i><em><strong><p>A</b>B</strong>C</em>D</i>", True),
    ("nested anchors and formatting", "<a><em>A<a>B</a>C</em>D", True),
    ("formatting across divs", "<b><div>A</div><div>B</b>C</div>", True),
    ("no furthest formatting", "<p><b>A<i>B</b>C</i>D", True),
    ("multiple paragraphs inside formatting", "<b><p>first<p>second</b>tail</p>", True),
    ("table formatting cell", "<table><tr><td><b>A<i>B</b>C</i></td></tr></table>", True),
    ("table formatting malformed", "<table><b><tr><td>A</td></tr>B</table>C", True),
    ("nested table formatting", "<table><tr><td><b>A<table><tr><td>B</b>C</td></tr></table></td></tr></table>", True),
    ("formatting after void element", "<b><img><i>A</b>B</i>", True),
    ("formatting link closing", "<a href=x><b>A</a>B</b>", True),
    ("formatting and special div", "<em><i><div>A</em>B</i>C</div>", True),
    ("formatting nolog", "<nobr><b>A<div>B</nobr>C</div>", True),
    ("misnested form", "<b><form><i>A</b>B</i></form>", True),
    ("H3C2 pending mixed text", "<table>  \n A<tr><td>B</td></tr></table>", False),
    ("H3C2 pending pure whitespace", "<table> \n <tr><td>B</td></tr></table>", False),
    ("H3C2 comment flush", "<table>hello<!--comment--><tr><td>X</td></tr></table>", False),
    ("H3C2 caption before row", "<table><caption><b>Title</b><tr><td>A</td></tr></table>", False),
    ("H3C2 caption closes with table", "<table><caption>A</table>B", False),
    ("H3C2 implied colgroup", "<table><col><col><tr><td>A</td></tr></table>", False),
    ("H3C2 explicit colgroup", "<table><colgroup><col></colgroup><tr><td>A</td></tr></table>", False),
    ("H3C2 head to body transition", "<table><thead><tr><th>A<tbody><tr><td>B</table>", False),
    ("H3C2 omitted cell close", "<table><tr><td>A<td>B</tr></table>", False),
    ("H3C2 caption with block", "<table><caption><p>Hello</p></caption><tr><td>A</td></tr></table>", False),
    ("H3C2 two table text segments", "<table>A<tr><td>B</td></tr>C</table>", False),
    ("H3C2 table whitespace and formatting", "<table> <b>X</b> </table>", False),


]
binary = sys.argv[1]
required_bad = 0
differences = 0
for label,html,required in cases:
    actual = subprocess.run([binary, html], capture_output=True, text=True, check=False)
    reference = expected_body_children(html)
    actual_lines = actual.stdout.splitlines() if actual.returncode == 0 else ["TRINITY_PARSE_ERROR"]
    if actual_lines != reference:
        differences += 1
        print(f"DIFF ({'REQUIRED' if required else 'EXPLORATORY'}): {label}")
        print("  WHATWG reference:", repr(reference))
        print("  Trinity:", repr(actual_lines))
        if required:
            required_bad += 1
    else:
        print(f"MATCH: {label}")
print(f"H3B2 html5lib differential: {len(cases)-differences}/{len(cases)} match; {required_bad} required mismatches")
sys.exit(1 if required_bad else 0)
