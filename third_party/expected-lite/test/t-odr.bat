cl -EHsc -DMAIN=""             -Dnsel_P0323R=-2 -I../include/nonstd/ -Foodr1.obj -c odr.cpp 
cl -EHsc -DMAIN="int main(){}" -Dnsel_P0323R=-2 -I../include/nonstd/ -Foodr2.obj -c odr.cpp
cl odr1.obj odr2.obj
