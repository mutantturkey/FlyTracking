== This is the least square solution CCI calulator ==

You'll need to run this in matlab, and have prepared your CSV's in advance. You
should have two CSV's. One csv should be all of your concatenated feature
vectors. One feature per column,  columns, and as many rows as you have
specimen. The second CSV should include the actual CI data recorded and
calculated by the biologists.

In matlab the command may look like this: (note you need to be in the directory
to call this)

    [x, e] = LeastSquareSolution('../input/ALL_FVs.csv','ALL_CIs.csv',
../output/);

Alternatively you can run it without first invoking the compiler by running
this:

    matlab -r "LeastSquareSolution  ALL_FV.csv ALL_CIs.csv ../output"


The ouput will an output specified, and will be in text files that are named
like this: Fold_1.txt Fold_2.txt etc. You will find it somewhat annoying to have
to concatenate all that data manually so here is a nice one-liner that allows
you to concatenate all of your data. For the number of specimen in each fold,
you need to setup your variables accordingly.

If we have 21 specimen per fold, use number of folds + 2 for the head argument,
and the number of folds for the tail. This will give you proper results!

    for i in Fold_*.txt; do head $i -n 23 |  tail -n 21 >> output.txt; echo $i; done;

The output of the file is in the order of your input, so just copy your data
back into whatever spreadsheet you are using and it be good. If you notice
that the numbers don't match up, make sure your ordering correctly!

