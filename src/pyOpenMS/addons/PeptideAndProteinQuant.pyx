from libcpp.map cimport map as libcpp_map
from cython.operator cimport dereference as deref, preincrement as inc


    def getPeptideResults(self):
        """
        getPeptideResults(self: PeptideAndProteinQuant) -> dict
        
        Get peptide abundance data.
        
        Returns a dictionary mapping peptide sequences (as strings) to PeptideData objects.
        """
        # Get the C++ map (returned by reference)
        cdef PeptideQuant _r = self.inst.get().getPeptideResults()
        py_result = dict()
        cdef PeptideQuant_iterator it__r = _r.begin()
        cdef PeptideAndProteinQuant_PeptideData item_py_result
        cdef AASequence key_seq
        while it__r != _r.end():
            # Get the AASequence key and convert to string
            key_seq = deref(it__r).first
            key_str = key_seq.toString()
            
            # Create Python wrapper for PeptideData value
            item_py_result = PeptideAndProteinQuant_PeptideData.__new__(PeptideAndProteinQuant_PeptideData)
            item_py_result.inst = shared_ptr[_PeptideAndProteinQuant_PeptideData](new _PeptideAndProteinQuant_PeptideData(deref(it__r).second))
            
            py_result[(<bytes>key_str.c_str()).decode('utf-8')] = item_py_result
            inc(it__r)
        return py_result

    def getProteinResults(self):
        """
        getProteinResults(self: PeptideAndProteinQuant) -> dict
        
        Get protein abundance data.
        
        Returns a dictionary mapping protein accessions (as strings) to ProteinData objects.
        """
        # Get the C++ map (returned by reference)
        cdef ProteinQuant _r = self.inst.get().getProteinResults()
        py_result = dict()
        cdef ProteinQuant_iterator it__r = _r.begin()
        cdef PeptideAndProteinQuant_ProteinData item_py_result
        while it__r != _r.end():
            # Get the String key and convert to Python string
            key_str = <bytes>deref(it__r).first.c_str()
            
            # Create Python wrapper for ProteinData value
            item_py_result = PeptideAndProteinQuant_ProteinData.__new__(PeptideAndProteinQuant_ProteinData)
            item_py_result.inst = shared_ptr[_PeptideAndProteinQuant_ProteinData](new _PeptideAndProteinQuant_ProteinData(deref(it__r).second))
            
            py_result[key_str.decode('utf-8')] = item_py_result
            inc(it__r)
        return py_result
