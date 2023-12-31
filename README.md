# Operating-System-Capstone

## Lab0
### Environment Setup
1. Cross Compiler : rpi3(樹梅派3)使採用ARM架構的CPU，因此在我們電腦上開發需要交叉編譯，並通過QEMU模擬器測試。
1. ELF(Executable and Linkable Format): 一種通用的二進制文件格式。
1. 樹莓派3的引導加載程序(bootloader )無法加載(load)ELF文件。因此需用objcopy將ELF轉成原始的二進制文件。

Source code -------> object file -------> ELF file ---------> Kernel Image(原始的二進制)  
 　　　　　(compiler)　　 　  　(Linker)　　　　(objcopy)
## Lab1


* Mini UART: 是樹莓派上的一個串行通信接口，通常用於在裸機編程或嵌入式開發中進行串口通信。
 在設置 UART 之後，實現一個簡單的 shell，讓樹莓派3與主機電腦(host)進行交互(互動)。

* ARM CPU 可以通過(mailbox)調用 VideoCore IV（GPU）的例程來配置外設，獲取硬體信息。
 
* 當程序被加載時，他需要：　
1. 所有的數據(data)都放到正確的內存地址(memory address)。
1. 程序計數器(program counter)被設置到正確的內存地址(memory address)。
1. bss section 初始化為 0。
1. 堆棧指針(stack pointer)被設置到合適的地址。

## Lab2

開機是在計算機重置後設置環境以運行各種用戶程序的過程。它包括由引導加載的內核、子系統初始化、設備驅動程序匹配以及加載 init 用戶程序以啟動用戶空間中的其餘服務。



SoC: System on a Chip   
ROM: Read-Only Memory  
以下為內核開始執行之前的四個步驟。  
1. GPU從SoC的ROM中執行第一階段的引導加載程序(bootloader)。
1. 第一階段的引導加載程序識別FAT16/32文件系統，從SD卡加載第二階段引導加載程序bootcode.bin到L2緩存。
1. bootcode.bin初始化SDRAM並加載start.elf。
1. start.elf讀取配置，將內核和其他數據加載到內存中，然後喚醒CPU以開始執行。

實做一個bootloader通過UART加載host上的kernel images，避免使用SD卡加載bootloader。可以消除使用SD卡在host與rpi3來回移動。

在kernel初始化後，它會掛載一個根文件系統並運行一個 init(初始化) 用戶程序。但是尚未實現任何文件系統和存儲驅動程序代碼，因此無法使用內核從SD卡加載任何內容。另一種方法是通過初始ramdisk加載用戶程序。初始ramdisk是由引導加載程序加載或嵌入在內核中的文件。它通常是一個可以提取的存檔，用於構建根文件系統。


### 新的ASCII格式Cpio存檔
Cpio是一種非常簡單的存檔格式，用於將目錄和文件打包。每個目錄和文件都被記錄為一個頭部，後跟其路徑名和內容。使用新的ASCII格式Cpio格式來創建一個cpio存檔。您可以首先創建一個rootfs目錄，並將所有需要的文件放在其中。我們實作一個parser(解析)來讀取該cpio存檔內的文件。

### Simple Allocator 
在子系統初始化的過程中，內核需要一個分配器。然而，動態分配器本身也是一個需要初始化的子系統。因此，在啟動的早期階段，我們需要一個簡單的分配器。

### Devicetree 
在啟動過程中，內核應該知道當前連接了哪些設備，並使用相應的驅動程序來初始化和訪問它。對於較強大的匯流排，如PCIe和USB，內核可以通過查詢匯流排的寄存器來檢測連接了哪些設備。然後，它將設備的名稱與所有驅動程序進行匹配，並使用兼容的驅動程序來初始化和訪問設備。

然而，對於一個帶有簡單匯流排的計算機系統，內核無法檢測連接了哪些設備。在處理這些設備的方法之一是像在實驗1中所做的那樣；開發人員知道要運行的目標機器是什麼，並在其內核中硬編碼IO內存地址。這導致驅動程序代碼不具可移植性。

一種更清晰的方法是使用描述計算機系統上有哪些設備的文件。該文件還記錄了每個設備的屬性和關係。然後，內核可以查詢這個文件，就像對強大的匯流排系統進行查詢以加載正確的驅動程序一樣。這個文件被稱為 "devicetree"。

Format

DeviceTree 有兩種格式，即 DeviceTree Source（DTS）和 Flattened DeviceTree（DTB）。DeviceTree Source 以人類可讀的形式描述設備樹，然後編譯成 Flattened DeviceTree，以便在性能較低的嵌入式系統中進行更簡單和更快的解析。

可以從樹莓派的 Linux 存儲庫中讀取樹莓派3的 DTS。
通過手動編譯或下載現成的 DTB 文件來獲取樹莓派3的 DTB。

Parsing
實現一個解析器(Parser)來解析Flattened DeviceTree。此外，內核提供一個接口，接受一個回調函數參數。因此，驅動程序代碼可以遍歷整個設備樹，查詢每個設備節點並通過檢查節點的名稱和屬性來進行匹配。


Dtb Loading
引導加載(bootloader)程序將一個 DTB 加載到內存中，並將指定在寄存器 x0 中的加載地址傳遞給內核。此外，它修改原始的 DTB 內容以匹配實際的機器設置。例如，如果您要求引導加載程序加載初始 ramdisk，則它將初始 ramdisk 的加載地址添加到 DTB 中。
## Lab3 Exception and Interrupt

### Exception Levels
最小權限原則限制了程序可以訪問的資源。這種限制減少了執行過程中可能發生的錯誤和攻擊面，從而提高了系統的安全性和穩定性。Armv8-A的CPU遵循這一原則並實現了異常層級，因此操作系統可以運行各種不同的用戶應用程序而不會使整個系統崩潰。

Armv8-A擁有4個異常層級（ELs）。通常，所有用戶程序運行在EL0中，操作系統運行在EL1中。操作系統可以通過設置系統寄存器並執行異常返回指令來降低異常層級並跳轉到用戶程序。當在用戶程序執行期間發生異常時，異常層級會增加，CPU將跳轉到異常處理程序。
![image](https://hackmd.io/_uploads/ry16QEyw6.png)


當CPU發生異常時，它執行以下操作：
1. 將當前處理器狀態（PSTATE）保存在SPSR_ELx中（x是目標異常層級）。
1. 將異常返回地址保存在ELR_ELx中。
1. 禁用中斷（PSTATE.{D,A,I,F}被設置為1）。
1. 如果異常是同步異常或SError中斷，將該異常的原因保存在ESR_ELx中。
1. 切換到目標異常層級並從相應的向量地址開始執行。

異常處理程序完成後，它使用eret發出返回異常的指令。然後CPU執行以下操作：
1. 從ELR_ELx中還原程序計數器。
1. 從SPSR_ELx中還原PSTATE。
1. 根據SPSR_ELx切換到相應的異常層級。

### Vector Table
CPU在相應的向量地址開始執行。該地址被定義為以下向量表，並且表的基地址保存在VBAR_ELx中。表的左半部分是異常發生的原因。表的右半部分是異常發生的EL和其目標EL之間的關係。
![image](https://hackmd.io/_uploads/ryB7DVJvp.png)



## Lab4 Allocator
內核為維護其內部狀態和用戶程序使用而分配物理內存。如果沒有內存分配器，您需要在物理內存中靜態分割出多個內存池，用於不同的對象。這對於在已知設備上運行已知應用程序的一些系統是足夠的。然而，運行各種應用程序和不同設備的通用操作系統需要在運行時確定物理內存的使用和量。因此，動態內存分配是必要的。

### Buddy system
Buddy system是一個眾所周知且簡單的連續內存塊分配算法。它存在內部碎片問題，但對於頁框分配(page frame allocation)仍然合適，因為該問題可以通過動態內存分配器來減少。

![image](https://github.com/b0938930511/Operating-System-Capstone/assets/50416832/17a0c8ff-43ad-4270-8c71-c8f5cacae880)


### Dynamic Memory Allocator

頁框分配器已經提供了大塊連續內存分配的功能。動態內存分配器只需添加一個包裝器，將頁框轉換為其物理地址。對於小內存分配，可以創建一些常見大小的內存池，例如[16, 32, 48, 96 …]。然後，將頁框劃分為幾個區塊槽。當有內存分配請求時，將所需的分配大小向上取整到最接近的大小，並檢查是否有未分配的槽。如果沒有，則從頁框分配器中分配一個新的頁框。然後，將一個區塊返回給調用者。來自同一頁框的對象具有共同的前綴地址。分配器可以使用它來確定在釋放時該區塊屬於的內存池。

## Lab5 Thread and User Process

一個 CPU 線程一次只能運行一個線程，但同時可能有多個可運行的線程。這些可運行的線程被放置在運行隊列中。當當前線程放棄對 CPU 線程的控制時，它調用調度器來選擇下一個線程。然後，一段代碼保存 CPU 線程的寄存器集並加載下一個線程的寄存器集。

在線程的執行過程中，它可能需要等待某個資源（例如，一個被鎖定的互斥鎖或一個未就緒的 IO 設備）。與忙碌等待相比，一種更有效的方法是讓出 CPU 線程，以便其他線程可以執行有意義的工作。然而，僅僅放棄 CPU 是不夠的，因為該線程可能再次被調度並浪費 CPU 時間。因此，當一個線程需要長時間等待時，它會將自己從運行隊列中刪除，將自己放入等待隊列中，並等待其他線程將其喚醒。

一般來說，每個資源都有自己的等待隊列。當資源就緒時，等待隊列中的一個或多個等待線程將被放回運行隊列。被喚醒的線程最終被調度並運行。然后，如果該資源仍然可用，它就可以獲取該資源。

### tpidr_el1: 
ARM 架構中的系統寄存器，保存了執行緒的本地數據指標（Thread Pointer Identifier Register EL1）。在上下文切換時，tpidr_el1 被用來獲取當前執行緒的數據結構，以便在調度器的 schedule() API 中進行切換。這樣的設計使得系統能夠有效地管理和切換不同執行緒的上下文。

### Yield and Preemption
一個線程可以自願將 CPU 線程讓給其他線程。然而，我們不能依賴自願放棄，因為一旦一個線程永不放棄，即使它是可運行的，高優先級線程也無法運行。因此，內核應該能夠強制當前線程放棄 CPU 線程（即抢占）。

抢占的實現很簡單。一旦一個線程在執行過程中放棄對 CPU 線程的控制，另一段代碼有機會調用調度器並切換到另一個線程。例如，當處於內核模式的線程被中斷時，控制權被轉交給中斷處理程序。在返回到原始執行之前，內核可以調用調度器執行上下文切換以實現內核抢占。當用戶進程發生異常（系統調用、中斷等）時，控制權被轉交給異常處理程序。在返回到原始執行之前，內核可以調用調度器執行上下文切換以實現用戶抢占。

## Lab6 virtual memory


虛擬內存提供了隔離的地址空間，因此每個用戶進程都可以在其地址空間中運行，而不會干擾其他進程。
初始化內存管理單元(memory management unit，MMU）並為內核和用戶進程設置地址空間，以實現進程隔離。
## Lab7 Virtual File System


文件系統管理存儲媒體中的數據。每個文件系統都有一種特定的方法來存儲和檢索數據。因此，在通用操作系統中，虛擬文件系統（VFS）是常見的，為所有文件系統提供統一的接口。
為內核實現一個VFS接口，以及一個基於內存的文件系統（tmpfs），作為根文件系統掛載。
