make clean

make

cd build

#pintos -v -k -T 10 -m 20   --fs-disk=10 -p tests/userprog/args-multiple:args-multiple -- -q   -f run "args-multiple some arguments for you!"

pintos -v -k -T 10 -m 20   --fs-disk=10 -p tests/userprog/args-multiple:args-multiple -- -q   -f run "args-multiple some arguments for 123"


#pintos -v -k -T 10 -m 20   --fs-disk=10 -p tests/userprog/args-many:args-many -- -q   -f run "args-many a b c d e f g h i z k l m"

#pintos -T 20 --fs-disk=10 -p tests/userprog/args-single:args -- -q -f run 'args oneagu'
#pintos  -T 10 --fs-disk=10 -p tests/userprog/args-many:echo -- -q -f run 'echo x y z'
#pintos --gdb --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single oneagu'

stty sane

