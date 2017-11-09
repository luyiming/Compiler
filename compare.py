import sys
import re

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: compare.py file1 file2')
        sys.exit()
    with open(sys.argv[1]) as f1, open(sys.argv[2]) as f2:
        for lineno, (line1, line2) in enumerate(zip(f1, f2)):
            m1 = re.match(r'Error type ([AB]) at Line (\d+):.*', line1)
            m2 = re.match(r'Error type ([AB]) at Line (\d+):.*', line2)
            if m1 is not None and m2 is not None:
                if m1.group(1) != m2.group(1) or m1.group(2) != m2.group(2):
                    print('Fail: Line {} File: {} {}'.format(lineno + 1, sys.argv[1], sys.argv[2]))
                    sys.exit()
            elif line1.strip() != line2.strip():
                print('Fail: Line {} File: {} {}'.format(lineno + 1, sys.argv[1], sys.argv[2]))
                sys.exit()
    print('pass {}'.format(sys.argv[1]))
