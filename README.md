
<img width="1950" height="1300" alt="路径追踪" src="https://github.com/user-attachments/assets/49f29c23-ee26-4ca6-b0ce-58960e46adda" />
<img width="1950" height="1300" alt="DDGI" src="https://github.com/user-attachments/assets/c46093ac-1209-4182-b581-90e0ad3777fe" />
<img width="1950" height="1287" alt="视锥剔除" src="https://github.com/user-attachments/assets/e65502fd-3b88-4bc5-a1f0-9004775ed9fb" />
<img width="1950" height="1260" alt="大气渲染" src="https://github.com/user-attachments/assets/80e010f8-0572-43e3-b697-a1b9d415021a" />
<img width="1950" height="1260" alt="IBL" src="https://github.com/user-attachments/assets/52eba6ae-906a-4b8d-8ab8-8b6775097786" />
<img width="1950" height="1299" alt="漫反射" src="https://github.com/user-attachments/assets/b7af29b1-8579-46f2-b377-f43ed5708bca" />
<img width="1950" height="1300" alt="镜面场景" src="https://github.com/user-attachments/assets/56cb3ade-f48a-4a0c-8f29-62b481f21717" />
<img width="1950" height="1330" alt="pathTracing" src="https://github.com/user-attachments/assets/88ca0cbf-66ad-4318-9cf9-b4ced570d9d0" />





# 多线程渲染框架
1. 主线程：计算逻辑，通过RENDER_SUBMIT()提交渲染命令缓存
2. 每次收集完一帧的命令后，Kick()渲染线程开始把渲染命令全部执行，同时主线程开始收集下一帧的渲染命令（单线程模式，等待渲染命令执行完后才开始收集下一帧命令）
3. 所以：上一帧的渲染命令执行和本帧的渲染命令收集是并发执行的 （所有有两个队列，一个负责让渲染线程执行，一个负责收集命令，每一帧进行交换）
4. 整个系统的设计的并发有两层：（CPU 主线程 ↔ 渲染线程并行）  （渲染线程 ↔ GPU并行（多帧飞行技术））
5. 资源释放问题：比如执行前一帧时，想释放某个资源，但是下一帧使用该资源的命令已经被缓存，当下一帧执行时，就会出现空指针（目前的解决思路是，延迟释放，如果渲染线程想释放某个资源，是把释放命令放在一个缓存中，过3帧后才释放，目前这样设计没有报错，Resize时涉及大量VkImage、FrameBuffer的释放，也没有出现问题）
