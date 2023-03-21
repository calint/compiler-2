int main(){
	int a=-2;
	int b=-a;
	if( !(a==-2 && b==-a) ) return 1;
	if( !(a==-2) || !(b==-a) ) return 2;
	if( !(a==-2 || b==-a) ) return 3;
	if( !(a==-2) && !(b==-a) ) return 4;
	if( !(a==-2 && !(a==-1 && b==-1))) return 5;
	if( !(a==-2 && !(a==-1 && b==-a))) return 6;
	if( !(a==-2 || !(a==-2 || b==-a))) return 7;
	return 0;
}
