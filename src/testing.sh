RECVD=`grep RECVD q1.log | wc -l` ; echo "RECVD: $RECVD";
IWANT=`grep IWANT u1.log | wc -l` ; echo "IWANT: $IWANT";
echo "----------------";
ENTER=`grep ENTER q1.log | wc -l` ; echo "ENTER: $ENTER";
IAMIN=`grep IAMIN u1.log | wc -l` ; echo "IAMIN: $IAMIN";
echo "----------------";
n2LATE=`grep 2LATE q1.log | wc -l` ; echo "2LATE: $n2LATE";
nCLOSD=`grep CLOSD u1.log | wc -l` ; echo "CLOSD: $nCLOSD";
echo "----------------";
TIMUP=`grep TIMUP q1.log | wc -l` ; echo "TIMUP: $TIMUP";
echo "----------------";
FAILD=`grep FAILD u1.log | wc -l` ; echo "FAILD: $FAILD";
echo "----------------";
GAVUP=`grep GAVUP q1.log | wc -l` ; echo "GAVUP: $GAVUP";

echo "----------------";
((passed = 5))
((faild = 0))

if [ $RECVD -ne $IWANT ]; then
    echo "RECVD failed IWANT";
    ((faild++));
fi

if [ $ENTER -ne $IAMIN ]; then
    echo "ENTER failed IAMIN";
    ((faild++));
fi

if [ $n2LATE -ne $nCLOSD ]; then
    echo "2LATE failed CLOSD";
    ((faild++));
fi

if [ $TIMUP -ne $ENTER ] || [ $TIMUP -ne $IAMIN ]; then
    echo "TIMUP failed ENTER, IAMIN";
    ((faild++));
fi

((sum = $n2LATE + $ENTER))
if [ $sum -ne $RECVD ]; then
    echo "2LATE + ENTER failed RECVD";
    ((faild++));
fi

((passed = passed - faild));
echo "TESTS PASSED $passed FROM 5";