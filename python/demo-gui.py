import tkinter as tk
import time
from opcua import Client
import mysql.connector
import datetime
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

# Enter Simulation Server URL
url = ""; 



# Database connectivity
mydb = mysql.connector.connect(
  host="", 
  user="",
  passwd="",
  database=""
)

mycursor = mydb.cursor()

client = Client(url)

client.connect()

# GUI Code
root = tk.Tk()

root.configure(background='#00bbff')

clabel = tk.Label(root, text="Generate OPC UA Values")
clabel.pack()
clabel.configure(background='white')
clabel.place(x=500, y=10, height=100, width=500)

clabel1 = tk.Label(root, text="Counter Value : ",padx=10, pady=10)
clabel1.pack()
clabel1.place(x=500, y=200)
clabel1.configure(background='white')

label1 = tk.Label(root, fg="black")
label1.pack() 
label1.configure(background='#00bbff')
label1.place(x=500, y=250)

hlimit1 = tk.Label(root)
hlimit1.pack()
hlimit1.place(x=500, y=300)
hlimit1.configure(background='#00bbff', fg='black')

r2 = tk.Label(root, text="> 0.5")
r2.pack()
r2.place(x=400, y=300)
r2.configure(background="white")

clabel2 = tk.Label(root, text="Sinusoidal Value : ", fg="white",padx=10, pady=10)
clabel2.pack()
clabel2.place(x=700, y=200)
clabel2.configure(background='green')

label2 = tk.Label(root, fg="black")
label2.pack() 
label2.configure(background='#00bbff')
label2.place(x=700, y=250)

hlimit2 = tk.Label(root)
hlimit2.pack()
hlimit2.place(x=700, y=300)
hlimit2.configure(background='#00bbff', fg='black')

clabel3 = tk.Label(root, text="Random Value : ", fg="white",padx=10, pady=10)
clabel3.pack()
clabel3.place(x=900, y=200)
clabel3.configure(background='green')

label3 = tk.Label(root, fg="black")
label3.pack() 
label3.configure(background='#00bbff')
label3.place(x=900, y=250)

hlimit3 = tk.Label(root)
hlimit3.pack()
hlimit3.place(x=900, y=300)
hlimit3.configure(background='#00bbff', fg='black')

clabel4 = tk.Label(root, text="Sawtooth Value : ", fg="white",padx=10, pady=10)
clabel4.pack()
clabel4.place(x=1100, y=200)
clabel4.configure(background='green')

label4 = tk.Label(root, fg="black")
label4.pack() 
label4.configure(background='#00bbff')
label4.place(x=1100, y=250)

hlimit4 = tk.Label(root)
hlimit4.pack()
hlimit4.place(x=1100, y=300)
hlimit4.configure(background='#00bbff', fg='black')

clabel5 = tk.Label(root, text="Triangle Value : ", fg="white",padx=10, pady=10)
clabel5.pack()
clabel5.place(x=1300, y=200)
clabel5.configure(background='green')

label5 = tk.Label(root, fg="black")
label5.pack() 
label5.configure(background='#00bbff')
label5.place(x=1300, y=250)

hlimit5 = tk.Label(root)
hlimit5.pack()
hlimit5.place(x=1300, y=300)
hlimit5.configure(background='#00bbff', fg='black')

c2 = 0
c3 = 0
c4 = 0
c5 = 0

counterArray = []
sineArray = []
randomArray = []
sawtoothArray = []
triangleArray = []
timeArray = []

sinet1 = ""
sinet2 = ""
randomt1 = ""
randomt2 = ""
sawtootht1 = ""
sawtootht2 = ""
trianglet1 = ""
trianglet2 = ""

temp1 = 1
temp2 = 1
temp3 = 1
temp4 = 1

countsine = 0
countrandom = 0
countsawtooth = 0
counttriangle = 0

def counter_label1():
    global timeArray
    global sineArray
    global randomArray
    global sawtoothArray
    global triangleArray
    global countsine
    global countrandom
    global countsawtooth
    global counttriangle
    
    named_tuple = time.localtime() # get struct_time
    time_string = time.strftime("%H:%M:%S", named_tuple)
    timeArray.append(time_string)
    
    button1.configure(state='disabled')
    Counter = client.get_node("ns=5;s=Counter1")
    counter = Counter.get_value()
    label1.config(text=str(counter))
    
    counterArray.append(counter)
    
    Sine = client.get_node("ns=5;s=Sinusoid1")
    sine = Sine.get_value()
    label2.config(text=str(sine))
    
    sineArray.append(sine)
    
    if sine > 0.5 :
        if countsine == 0 :
            global sinet1
            sinet1 = datetime.datetime.now()
        countsine += 1
        clabel2.configure(background='red')
    else :
        global sinet2
        global temp1
        if countsine is not 0 :   
            sinet2 = datetime.datetime.now()
            #timediff = sinet2-sinet1
            formatted_datetime1 = sinet1.strftime('%Y-%m-%d %H:%M:%S')
            formatted_datetime2 = sinet2.strftime('%Y-%m-%d %H:%M:%S')
            sql1 = "INSERT INTO sinealert(Number,Generation,Checked,Downtime) VALUES (%d,'%s','%s','%s')"
            val1 = (temp1,formatted_datetime1,formatted_datetime2,countsine)
            mycursor.execute(sql1 %val1)
            temp1 += 1 
        clabel2.configure(background='green')
        countsine = 0
            
    Random = client.get_node("ns=5;s=Random1")
    random = Random.get_value()
    label3.config(text=str(random))
    
    randomArray.append(random)
    
    if random > 0.5 :
        if countrandom == 0 :
            global randomt1
            randomt1 = datetime.datetime.now()
        countrandom += 1
        clabel3.configure(background='red')
    else :
        global randomt2
        global temp2
        if countrandom is not 0 :    
            randomt2 = datetime.datetime.now()
            #timediff = randomt2-randomt1
            formatted_datetime1 = randomt1.strftime('%Y-%m-%d %H:%M:%S')
            formatted_datetime2 = randomt2.strftime('%Y-%m-%d %H:%M:%S')
            sql2 = "INSERT INTO randomalert(Number,Generation,Checked,Downtime) VALUES (%d,'%s','%s','%s')"
            val2 = (temp2,formatted_datetime1,formatted_datetime2,countrandom)
            mycursor.execute(sql2 %val2)
            temp2 += 1
        clabel3.configure(background='green')
        countrandom = 0
        
    Sawtooth = client.get_node("ns=5;s=Sawtooth1")
    sawtooth = Sawtooth.get_value()
    label4.config(text=str(sawtooth))
    
    sawtoothArray.append(sawtooth)
    
    if sawtooth > 0.5 :
        if countsawtooth == 0 : 
            global sawtootht1
            sawtootht1 = datetime.datetime.now()
        countsawtooth += 1
        clabel4.configure(background='red')
    else :
        global sawtootht2
        global temp3
        
        if countsawtooth is not 0 :  
            sawtootht2 = datetime.datetime.now()  
            #timediff = sawtootht2-sawtootht1
            formatted_datetime1 = sawtootht1.strftime('%Y-%m-%d %H:%M:%S')
            formatted_datetime2 = sawtootht2.strftime('%Y-%m-%d %H:%M:%S')
            sql3 = "INSERT INTO sawtoothalert(Number,Generation,Checked,Downtime) VALUES (%d,'%s','%s','%s')"
            val3 = (temp3,formatted_datetime1,formatted_datetime2,countsawtooth)
            mycursor.execute(sql3 %val3)
            temp3 += 1
        clabel4.configure(background='green')
        countsawtooth = 0
        
    Triangle = client.get_node("ns=5;s=Triangle1")
    triangle = Triangle.get_value()
    label5.config(text=str(triangle))
    
    triangleArray.append(triangle)
    
    if triangle > 0.5 :
        if counttriangle == 0 :
            global trianglet1
            trianglet1 = datetime.datetime.now()
        counttriangle += 1
        clabel5.configure(background='red')
    else :
        global trianglet2
        global temp4
        trianglet2 = datetime.datetime.now()
        if counttriangle is not 0 :    
            #timediff = trianglet2-trianglet1
            formatted_datetime1 = trianglet1.strftime('%Y-%m-%d %H:%M:%S')
            formatted_datetime2 = trianglet2.strftime('%Y-%m-%d %H:%M:%S')
            sql4 = "INSERT INTO trianglealert(Number,Generation,Checked,Downtime) VALUES (%d,'%s','%s','%s')"
            val4 = (temp4,formatted_datetime1,formatted_datetime2,counttriangle)
            mycursor.execute(sql4 %val4)
            temp4 += 1
        clabel5.configure(background='green')
        counttriangle = 0
        
    root.after(1000, counter_label1)
            
            
    sql = "INSERT INTO logfile1 (Counter,Sinusoidal,Random,Sawtooth,Triangle) VALUES (%d,%2.6f,%2.6f,%2.6f,%2.6f)"
    val = (counter,sine,random,sawtooth,triangle)
    mycursor.execute(sql %val)

    mydb.commit()
            
    timeArray = timeArray[-5:]
    sineArray = sineArray[-5:]
    randomArray = randomArray[-5:]
    sawtoothArray = sawtoothArray[-5:]
    triangleArray = triangleArray[-5:]
    
    a.plot(timeArray,sineArray,"b-*",timeArray,randomArray,"r-o")
    a.set_xlabel("Time")
    a.set_ylabel("Sinusoidal (Blue)        Random (Red)")
    a.grid()
    canvas.draw()
    
    
    b.plot(timeArray,randomArray,marker='o')
    b.set_xlabel("Time")
    b.set_ylabel("Random")
    b.grid()
    canvas2.draw()
    
    c.plot(timeArray,sawtoothArray,"b-*",timeArray,triangleArray,"r-o")
    c.set_xlabel("Time")
    c.set_ylabel("Sawtooth (Blue)       Triangle (Red)")
    c.grid()
    canvas3.draw()
    
    a.cla()
    b.cla()
    c.cla()

def on_enter(event):
    button1['background']="#06a2db"
    button1['fg']="white"
    
def on_leave(event):
    button1['background']="white"
    button1['fg']="#06a2db"
    
def on_enter2(event):
    button2['background']="#06a2db"
    button2['fg']="white"
    
def on_leave2(event):
    button2['background']="white"
    button2['fg']="#06a2db"
    
root.title("OPC Generator")

button1 = tk.Button(root, text="Start", width=25, command=counter_label1, padx=6, pady=6, font='Helvetica 10 bold ')
button1.configure(background="white", fg="#06a2db")
button1.pack()
button1.place(x=50,y=200)
button1.bind("<Enter>",on_enter)
button1.bind("<Leave>",on_leave)

button2 = tk.Button(root, text="Stop", width=25, command=root.destroy, padx=6, pady=6, font='Helvetica 10 bold ')
button2.configure(background="white", fg="#06a2db")
button2.pack()
button2.place(x=50,y=300)
button2.bind("<Enter>",on_enter2)
button2.bind("<Leave>",on_leave2)

figure = Figure(figsize=(6,5),dpi=70)
figure.suptitle("Sinusoidal v/s Random Visualization")

figure2 = Figure(figsize=(6,5),dpi=70)
figure2.suptitle("Random Visualization")

figure3 = Figure(figsize=(6,5),dpi=70)
figure3.suptitle("Sawtooth v/s Triangle Visualization")

a = figure.add_subplot(111)
b = figure2.add_subplot(111)
c = figure3.add_subplot(111)

canvas = FigureCanvasTkAgg(figure, master=root)
canvas2 = FigureCanvasTkAgg(figure2, master=root)
canvas3 = FigureCanvasTkAgg(figure3, master=root)
    
canvas.get_tk_widget().pack()
canvas.get_tk_widget().place(x=10,y=450)
canvas2.get_tk_widget().pack()
canvas2.get_tk_widget().place(x=550,y=450)
canvas3.get_tk_widget().pack()
canvas3.get_tk_widget().place(x=1100,y=450)

root.mainloop()
