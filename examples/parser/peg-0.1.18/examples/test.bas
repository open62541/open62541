10 let i=1
20 gosub 100
30 let i=i+1
40 if i<=10 then goto 20
50 end

100 let j=1
110 print " ", i*j,
120 let j=j+1
130 if j<=i then goto 110
140 print
150 return
