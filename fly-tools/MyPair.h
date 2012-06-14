class MyPair {

public:
	int first, second;
	MyPair(int x, int y) {
	
		this->first = x;
		this->second = y;
		
	}
	MyPair() {
		
		this->first = -1;
		this->second = -1;
		
	}
	MyPair(const MyPair &f) {
		this->first = f.getX();
		this->second = f.getY();
		
	}
	int getX() const {
		return this->first;
	}

	int getY() const {
		return this->second;
	}
	
};
