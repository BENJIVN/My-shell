# Authors: 
Benjamin Nguyen: bvn6
Victor Nguyen: vvn12 

# TESTPLAN:
# TESTCASES: 

## REDIRECTION: test1.txt 

opens/creates an output.txt file and writes all the files in the current working directory into it. then outputs the number of lines that are now in output.txt. shell reads the contents of inputfile.txt and writes it to outputfile.txt

```
ls > output.txt
wc - l < output.txt
cat < inputfile.txt > outputfile.txt
```

##  PIPING: test2.txt

prints all the contents of a text file and all directories in the current working directory. should display the # of words. the shell then lists only the files in the current directory with a .txt extension. 

```
cat a.txt | ls
echo hello world | wc -w
ls -l | grep .txt
```

## WILDCARDS: test3.txt 

takes all the txt files in the current working directory and sorts them and outputs it to sorted.txt. counts the number of .txt files in the current directory and redirects the list of .txt file to txtfiles.txt. combines/ concatenates all .txt files into combined.txt. lastly the shell lists all .txt files in both dsubir1 and subdir2. 

```
ls *.txt
ls *.txt | sort > sorted.txt
ls *.txt | wc -l
ls | grep *.txt > txtfile.txt
cat *.txt > combine.txt
ls subdir*/*.txt
```

## REIDRECION AND PIPING test4.txt

```

```

## test5.txt

    cd test --> test5.txt

## test6.txt

    pwd test --> test6.txt

## test7.txt

    conditionals test --> test7.txt

## test7.txt

    hello world --> test8.txt

