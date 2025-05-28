void main()
{
    __asm__ volatile("int $0x80");

end:
    for(;;);
}