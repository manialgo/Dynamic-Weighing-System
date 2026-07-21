# Dynamic Weighing System

**A Real-Time Conveyor Belt-Based Dynamic Weight Measurement System**

An innovative engineering project that measures the weight of objects **while they are in motion** on a custom-built conveyor belt system. Built entirely from scratch with hands-on mechanical fabrication, electronics, and programming.

---

## 🎯 Project Objective

To design and develop a **Dynamic Weighing System** capable of accurately measuring the weight of moving objects on a conveyor belt using a load cell, without stopping the object. The system achieves high precision of **±0.005 grams per kilogram**.

---

## 👥 Team Members

- **[Manikandan M](https://github.com/manialgo)**
- **[Aathithayn K](https://github.com/Aathithyan02)**
- **[Hiruthikesh K](https://github.com/Hiruthirosh555)**

**Duration**: 35 Days (Intense work over ~20 days of core development + 15 days of testing & refinement)

---

## ✨ Key Features

- Real-time weight measurement of objects **in motion**
- Custom-designed triangular conveyor belt system
- Four DC motors with PWM speed control
- Load cell + plate structure for weight sensing
- LCD display for live weight output
- High precision: **±0.005 g/kg**
- Fully custom mechanical structure (cutting, welding, drilling done by team)

---

## 🛠️ System Architecture

### Mechanical Design
- Two-stool arrangement supporting a triangular conveyor belt
- Elastic rubber cloth belt fixed using custom cones and PVC rollers
- Ball bearings for smooth rotation
- Load cell mounted under the belt with a weighing platform
- Object drops slightly due to gravity onto the weighing zone

### Electronics & Control
- Microcontroller-based control (Arduino UNO / Nano)
- PWM control for four motors
- HX711 Load Cell Amplifier
- LCD Display (16x2 or similar)
- Custom circuit design

### Working Principle
Objects move on the conveyor belt → pass over the load cell platform → weight is measured over a short duration → displayed on LCD.

---

## 📁 Repository Structure

Dynamic-Weighing-System/  
├── Stage-1/  
│   ├── Program/              # Source code (UNO & Nano versions)  
│   ├── Pictures/             # Circuit diagrams & photos  
│   └── Completion-Video-01 & 02  
├── Stage-2/  
│   └── Generated-Images/     # Additional visuals  
├── .gitignore  
└── README.md  


---

## 🛠️ Technologies & Tools Used

- **Mechanical**: PVC pipes, ball bearings, rubber belt, custom fabrication
- **Electronics**: Arduino, Load Cell + HX711, LCD Display, Motors & Drivers
- **Programming**: C++ (Arduino)
- **Tools**: Welding, Drilling, Cutting, Multimeter, Oscilloscope (for testing)

---

## 📸 Gallery

*(Images and completion videos are available in the repository under `Stage-1/`)*

- Circuit Diagram
- Mechanical Assembly Photos
- Working Videos (Completion-Video-01 & 02)

---

## 🚀 Challenges Overcome

- Sourcing correctly sized ball bearings (took over a week)
- Achieving stable belt movement and tension
- Calibrating load cell for moving objects
- Noise reduction and vibration isolation
- Multiple iterations and 15 days of rigorous testing & debugging

**Note**: A significant portion (≈75%) of the source code was initially generated with AI assistance and then refined by the team.

---

## 🎉 Achievements

- Successfully built a fully functional **Dynamic Weighing System** from scratch.
- Achieved excellent accuracy of **±0.005 grams per kilogram**.
- Complete ownership of mechanical fabrication, electronics, and software.

---
