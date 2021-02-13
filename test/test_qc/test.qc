void(float n) counter_increase = #0;
void(string msg) print = #0;

void() test_main =
{
    counter_increase(2);
    print("Hello from QC");
};