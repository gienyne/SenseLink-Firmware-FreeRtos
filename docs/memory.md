# Memory Optimisation

The STM32F030R8 provides only **8 KB of SRAM**, making memory management one of the primary engineering challenges of this project.

The FreeRTOS heap must coexist with:

* Global variables
* HAL drivers
* Interrupt stack
* FreeRTOS task stacks
* Queues
* Mutexes
* Task Control Blocks (TCBs)

---

# Heap Configuration

The project uses the following FreeRTOS heap configuration:

```text
configTOTAL_HEAP_SIZE = 3972 bytes
```

This value was determined experimentally to maximise available memory while leaving sufficient space for the remaining firmware components.

---

# Task Stack Allocation

| Task       | Stack Size |
| ---------- | ---------: |
| TaskSensor |  256 words |
| TaskLCD    |  128 words |
| TaskUART   |  192 words |
| TaskAlarm  |   64 words |
| IDLE       |  128 words |

Total stack allocation:

```text
3072 bytes
```

---

# Remaining Heap

After allocating task stacks, approximately **900 bytes** remain available for:

* FreeRTOS queues
* Queue storage
* Task Control Blocks
* Mutexes
* Dynamic kernel objects

This margin proved sufficient for stable operation.

---

# Memory Optimisation Techniques

Several techniques were used to minimise RAM usage.

## Centralised Formatting

Only `TaskSensor` performs `snprintf()` operations.

Consumer tasks receive already formatted strings, avoiding unnecessary floating-point formatting and reducing stack requirements.

---

## Queue Sizing

Each queue was sized according to its functional requirements instead of using arbitrary values.

| Queue      | Depth | Reason                   |
| ---------- | ----: | ------------------------ |
| AlarmQueue |     1 | Latest measurement only  |
| UARTQueue  |     2 | Scheduling jitter        |
| LCDQueue   |     3 | I2C contention tolerance |

---

## Stack Tuning

Task stack sizes were progressively reduced through testing until reliable operation was achieved.

This approach maximised the remaining heap without compromising stability.

---

# Debugging Heap Exhaustion

During development, increasing task stack sizes beyond the available heap caused `osThreadCreate()` to return `NULL`.

This failure was diagnosed by checking every task handle immediately after creation and signalling failures using the onboard LED.

This debugging process ultimately led to the final memory configuration used in the project.
