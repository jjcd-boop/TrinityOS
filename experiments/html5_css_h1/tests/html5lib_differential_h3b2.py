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
    def children(element):
        if element.text:
            out.append("T:" + element.text)
        for node in element:
            out.append("E:" + str(node.tag))
            children(node)
            out.append("/E")
            if node.tail:
                out.append("T:" + node.tail)
    children(body)
    return out

cases = [
    ("basic misnested", "<b><i>1</b>2</i>", True),
    ("direct furthest block", "<b>1<p>2</b>3</p>", True),
    ("nested link", "<p><a href=x>A<a href=y>B</a>C</p>", False),
    ("intermediate italic", "<b><i><p>X</b>Y", False),
    ("intermediate closing", "<b><i><p>X</b>Y</i>Z</p>", False),
    ("multiple intermediate", "<b><em><i><div>X</div></b>Y</em>Z</i>", False),
    ("formatting attribute", "<em class=x><i><div>A</em>B</i></div>", False),
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
