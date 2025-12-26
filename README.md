# Pyoneer
Pyoneer is a distributed system designed to extend a local python environment to many machines. Since python programs range in complexity, Pyoneer defines a collection of resources that structure the program proportional to its needs. For example, if we need to run several related tasks, we can create a resource that bundles together those tasks together and execute them on a remote machine. The goal of the Pyoneer is to be very lightweight in configuration and to sit on top of a python environment :).

## Definitions

### Kind
A **kind** specifies the type of resource in the distributed system. For example, kind can equal job or project, where each value corresponds to a distinct resource type.

### Blueprint
A **blueprint** is a resource definition within the distributed system. Each blueprint has a unique kind and may be composed of other blueprints.

### Status
**Status** refers to the state of an agent and/or its resource.

- Examples of **agent status**: `assigned`, `not_working`
- Examples of **resource status**: `not_ready`, `ready`, `running`

### Pyoneer
A **pyoneer** is an agent in the distributed system. Each pyoneer has a unique role and a unique blueprint that it works on.

To avoid confusion with the name of the distributed system, we use:
- **Pyoneer** (uppercase) to refer to the distributed system
- **pyoneer** (lowercase) to refer to an individual agent

## Core Design

- The state of the distributed system is modelled as a **cartesian product** of each agent and resource state. 
  - *Informally:* the state of Pyoneer is the status of each pyoneer and its blueprint, and state transitions are pyoneers working on their assigned blueprints.
  - *Formally:* Let $V$ and $W$ denote the sets of all agent and resource states respectively. Then $$Q = \overbrace{V \times W \times \cdots \times V \times W}^{2n}$$ where $n$ denotes the number of agents and resources in the distributed system.

- There is a **1-to-1 relationship** between pyoneers and blueprints in the system.  
  - *Example:* a worker is assigned a job and a job is completed by a worker.

- Blueprints are **statusless** in isolation. A blueprint’s status is defined **only in the context of a pyoneer**.  
  - *Example:* a project (blueprint) is stateless until it's run by a manager (pyoneer).

- Projects are composed of **jobs** with dependencies. Jobs are composed of **tasks**, and tasks are implemented as **Python scripts**.

- All pyoneers must implement the following functions:  
  - `get_status`  
  - `run`  
  - `assign`  
  - `restart`  
  - `start`  
  - `stop`

- All blueprints must implement the following functions:  
  - `decode`  
  - `encode`  
  - `encode_status`  
  - `get_status`  
  - `run`

- All blueprint runners must implement the following functions:  
  - `restart`  
  - `stop`  
  - `wait`

- All pyoneer methods and API functions are **thread-safe**. That is, they may be called concurrently without data races, corruption, or undefined behavior.

- Managers communicate with workers, and workers communicate with the runtime engine. Workers do **not** communicate with other workers.

- Workers are **lazy**, that is, they idle after completing a job and wait for another job assignment.
