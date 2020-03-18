
#ifndef I_RENDERABLE_H
#define I_RENDERABLE_H

class IRenderable
{

public: 
	
	// Make a virtual destructor in case we delete an IRenderable pointer, 
	// so the proper derived destructor is called
	virtual			~IRenderable() {}; 
	
	virtual	void	render() = 0;

};


#endif