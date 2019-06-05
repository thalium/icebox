
set -x
python concatlines.py <concatlinesample >comparatorsample.new
diff comparatorsample comparatorsample.new

