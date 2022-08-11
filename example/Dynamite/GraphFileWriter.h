#include "context.h"

class GraphFileWriter {

    public: 
        virtual void writeToFile(Context& context) = 0;
};