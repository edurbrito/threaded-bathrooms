RECVD=`grep RECVD q2.log | wc -l` ; echo "RECVD: $RECVD";
IWANT=`grep IWANT u2.log | wc -l` ; echo "IWANT: $IWANT";
echo "----------------";
ENTER=`grep ENTER q2.log | wc -l` ; echo "ENTER: $ENTER";
IAMIN=`grep IAMIN u2.log | wc -l` ; echo "IAMIN: $IAMIN";
echo "----------------";
n2LATE=`grep 2LATE q2.log | wc -l` ; echo "2LATE: $n2LATE";
nCLOSD=`grep CLOSD u2.log | wc -l` ; echo "CLOSD: $nCLOSD";
echo "----------------";
TIMUP=`grep TIMUP q2.log | wc -l` ; echo "TIMUP: $TIMUP";
echo "----------------";
FAILD=`grep FAILD u2.log | wc -l` ; echo "FAILD: $FAILD";
echo "----------------";
GAVUP=`grep GAVUP q2.log | wc -l` ; echo "GAVUP: $GAVUP";

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