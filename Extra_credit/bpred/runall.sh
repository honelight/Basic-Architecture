
######################################################################################
# This scripts runs all three traces (A,B,C) for all predictors
# If you need to test only one predictor or only one trace, comment the other lines
# The ALWAYSTAKEN and NEVERTAKEN predictors are already commented
######################################################################################


# ./predictor 0  A.trace.gz > A.NeverTaken.res ;
# ./predictor 0  B.trace.gz > B.NeverTaken.res ;
# ./predictor 0  C.trace.gz > C.NeverTaken.res ;

# ./predictor 1  A.trace.gz > A.AlwaysTaken.res ;
# ./predictor 1  B.trace.gz > B.AlwaysTaken.res ;
# ./predictor 1  C.trace.gz > C.AlwaysTaken.res ;

./predictor 2  A.trace.gz > A.LastTime.res ;
./predictor 2  B.trace.gz > B.LastTime.res ;
./predictor 2  C.trace.gz > C.LastTime.res ;

# ./predictor 3  A.trace.gz > A.TwoBitCounter.res ; 
# ./predictor 3  B.trace.gz > B.TwoBitCounter.res ;
# ./predictor 3  C.trace.gz > C.TwoBitCounter.res ;

# ./predictor 4  A.trace.gz > A.TwoLevelPred.res ;
# ./predictor 4  B.trace.gz > B.TwoLevelPred.res ;
# ./predictor 4  C.trace.gz > C.TwoLevelPred.res ;

echo "All Done. Use \"grep PERCENTAGE *.res\" to gather CORRECT PREDICTION stats";

