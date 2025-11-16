# Synchronization Quest - Application 5

## Real Time Systems - Fall 2025
**Instructor:** Dr. Mike Borowczak

## Author
**Name:** Kelvin Vu

**Theme:** Space Systems 

Powerpoint slide: https://ucf-my.sharepoint.com/:p:/g/personal/ke088292_ucf_edu/IQDLxy6H0viZR4e_radjAXhjAVkpAqutzxnPhusjLt0HM7I?e=9w3rEq

### 1. Migration Strategy & Refactor Map

- Outline the three largest structural changes you had to make when porting the single-loop Arduino sketch to your chosen RTOS framework.
- Include one code snippet (before → after) that best illustrates the refactor.

**Answer:**  
The three largest structural changes I had to make when importing the single-loop Arduino sketch
to my chosen RTOS framework was formating the web server html, the print, and the task functions.

Before:

          div { display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>

        <div>
          <h2>LED 1</h2>
          <a href="/toggle/1" class="btn LED1_TEXT">LED1_TEXT</a>
          <h2>LED 2</h2>
          <a href="/toggle/2" class="btn LED2_TEXT">LED2_TEXT</a>

  
After:

    div { display: grid; grid-template-columns: 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>

        <div>
          <h2>Alarm</h2>
          <a href="/toggle/1" class="btn LED1_TEXT">LED1_TEXT</a>



### 2. Framework Trade-off Review

- Compare ESP-IDF FreeRTOS vs Arduino + FreeRTOS for this project: list two development advantages and one limitation you encountered with the path you chose.
- If you had chosen the other path, which specific API or tooling difference do you think would have helped / hurt?

**Answer:**

For the two approaches of the project, Arduino + FreeRTOS proved to be easy as the github resources
had functions and FreeRTOS seemed to work, but the one limitation was that I did not know Arduino code.
The ESP-IDF FreeRTOS mostly used the previous functions we used in previous applications
and was purely in FreeRTOS. When trying to run the example code and opening the local host
to prototype the system, an error occured stating that header was too  big. It might have
been an easy fix, but I did not delve too deply into the ESP-IDF FreeRTOS path because the other one 
seemed to work at first glance. If I had chosen the other path for Application 5, I think having it purely be in FreeRTOS
would have been more familar.

### 3. Queue Depth & Memory Footprint

- How did you size your sensor-data queue (length & item size)? Show one experiment where the queue nearly overflowed and explain how you detected or mitigated it.

**Answer:**  
I sized the sensor-data queue by using the one I tested in the previous applications. 
I decreased the MAX_COUNT_SEM, there would not be able to log as fast.

### 4. Debug & Trace Toolkit

- List the most valuable debug technique you used (e.g., esp_log_level, vTaskGetInfo,  print-timestamp).
- Show a short trace/log excerpt that helped you verify correct task sequencing or uncover a bug.

**Answer:**  
Using print timestamps is useful when determing the periodic timing of tasks and to see
if there is any delays or drift. Although the blink status is the lowest priority task so the 
logging does not have to perfect, if there is a noticable drift, it would mean that one of the 
higher priority tasks is taking more computation time than expected. 

        Status LED @ 6342 
        Status LED @ 7342 
        Status LED @ 8342 

### 5. Domain Reflection

- Relate one design decision directly to your chosen theme’s stakes (e.g., “missing two heart-rate alerts could delay CPR by ≥1 s”).
- Briefly propose the next real feature you would add in an industrial version of this system.

**Answer:**  

I made it that the an alert would go off once it sensed the radiation value going past the threshold.
The next real feature I would add to the system would be to make it play an alarm once if it 
senses it passed the the threshold for a certain amount of time. 

### 6. Web-Server Data Streaming: Benefits & Limitations

- Describe two concrete advantages of pushing live sensor data to the on-board ESP32 web server (e.g., richer UI, off-board processing, remote monitoring, etc.).
- Identify two limitations or trade-offs this decision introduces for a real-time system (think: added latency, increased heap/stack usage, potential priority inversion, Wi-Fi congestion, attack surface, maintenance of HTTP context inside RTOS tasks, etc.).
- Support your points with evidence from your own timing/heap logs or an experiment—e.g., measure extra latency or increased CPU load when the server is actively streaming data versus idle.
- Conclude with one design change you would implement if your system had to stream data reliably under heavy load

**Answer:**  

Two concrete advantages of pushing live sensor data to the on=board ESP32 web server 
is that it allows split on-board processing and an interactive screen. ESP32 has 
two cores, and the tasks are usually pinned to one core and the web server to the other.
Two trade-offs of using web servers are latency and priority. When testing the example
code, there was a noticable delay when toggling the LEDs. When running the simulation, the 
system first prioritized connecting to the web server, delaying all the other tasks. 
A design change I would implement if my system had to stream data reliably under 
heavy load would be to deprioritize the other tasks or lessen the load. 
