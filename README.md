# pcitools
pcitools are a group of tools I wrote during my deep-dive into PCI and PCI-express.
Some of the work was towards getting around Intel's DEVHIDE feature.  The program related to that is
thhe pcimmio.c program.  Instead of using the PIO x86 instructions (IN/OUT) it uses the memory at
0x80000000-0x90000000 with some bitshifting and arithmetic to find out exactly where it's bus, dev, and function/register
are located in memory.

The reason I attempted this method against Intel's DEVHIDE was because:

1) I didn't have much time to look into much else and this was, while a long shot, sensible in a way

2) It was sensible inasmuch as the query to the PCI(e) controller was coming from a different direction, and since the
DEVHIDE flag depends on the Root Hub/Controller to hide the PCI(e) device, I figured that 1) perhaps this method, which seems to be less common on x86 than other platforms like SPARC, MIPS, ARM, which have to use MMIO, would catch some data (the PCI(e) configuration space - 4096 bytes for PCIe and 256 bytes for PCI), that I could then use to walk to the ROM, and the PPD.

3) My work on this is not totally clean, I need to implement logic so that I don't dump config spaces of garbage memory.  I have a few conditionals to tell whether it is or not, but there needs to be quite a few more for reliable results.

4) Note that in another one of my programs (pcifindrom.c) which finds the PCI ROM signature and validates that it is indeed a ROM, it is possible to search memory for any references to this ROM and double check whether those references live inside PCI(e) configuration space.  I have not finished this yet, but this program is still useful (main thing is getting rid of garbage in the config space area being taken as config space).  So in essence, I find a 0xAA 0x55 PCI(e) ROM header.  Then I find certain values at offsets to verify it's a ROM HDR.  Then I take the address of the ROM HDR and find that in memory, looping... one I find a reference that looks like it's definitely from PCI(e) configuration space, I've "walked backwards" from ROM to conf space.  This is normally not done, because the ROM contains 0 pointers for the developer/user to get back to the configuration space.

