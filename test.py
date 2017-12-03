import sys
import re
import os
import subprocess
import itertools

def compare(true_file, output_lines):
    PATTERN = re.compile(r'Error type (.*) at Line (\d+):.*')
    with open(true_file) as f:
        for lineno, (line1, line2) in enumerate(itertools.zip_longest(f, output_lines, fillvalue="")):
            m1, m2 = re.match(PATTERN, line1), re.match(PATTERN, line2)
            if m1 is not None and m2 is not None:
                if m1.group(1) != m2.group(1) or m1.group(2) != m2.group(2):
                    print('Fail: Line {} File: {}'.format(lineno + 1, true_file))
                    print('  true   >>> ', line1.strip())
                    print('  output >>> ', line2.strip())
                    return
            elif line1.strip() != line2.strip():
                print('Fail: Line {} File: {}'.format(lineno + 1, true_file))
                print('  true   >>> ', line1.strip())
                print('  output >>> ', line2.strip())
                return
    print('pass {}'.format(true_file))

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: test.py path_to_bin directory')
        sys.exit()
    parser = sys.argv[1]
    directory = sys.argv[2]
    for filename in os.listdir(directory):
        src_file = os.path.join(directory, filename)
        if not filename.endswith(".true") and os.path.isfile(src_file):
            true_file = src_file + '.true'
            if not os.path.exists(true_file):
                print('missing true file for ' + src_file)
            else:
                output_lines = subprocess.Popen('{} {}'.format(parser, src_file), shell=True, stdout=subprocess.PIPE).stdout.readlines()
                output_lines = [line.decode('utf-8') for line in output_lines]
                compare(true_file, output_lines)
