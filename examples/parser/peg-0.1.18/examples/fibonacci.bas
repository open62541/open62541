100 let n=32
110 gosub 200
120 print "fibonacci(",n,") = ", m
130 end

200 let c=n
210 let b=1
220 if c<2 then goto 400
230 let c=c-1
240 let a=1
300 let c=c-1
310 let d=a+b
320 let a=b
330 let b=d+1
340 if c<>0 then goto 300
400 let m=b
410 return
