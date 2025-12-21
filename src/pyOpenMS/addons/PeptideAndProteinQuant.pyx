from libcpp.map cimport map as libcpp_map
from cython.operator cimport dereference as deref, preincrement as inc


    def getPeptideResults(self):
        """
        getPeptideResults(self: PeptideAndProteinQuant) -> dict
        
        Get peptide abundance data.
        
        Returns a dictionary mapping peptide sequences (as strings) to PeptideData objects.
        """
        cdef libcpp_map[_String, PeptideAndProteinQuant_PeptideData] c_result
        c_result = self.inst.get().getPeptideResultsAsStringKeys()
        
        result = {}
        cdef libcpp_map[_String, PeptideAndProteinQuant_PeptideData].iterator it = c_result.begin()
        while it != c_result.end():
            key_str = <bytes>deref(it).first.c_str()
            
            # Create Python wrapper for PeptideData
            py_data = PeptideAndProteinQuant_PeptideData.__new__(PeptideAndProteinQuant_PeptideData)
            py_data.inst = shared_ptr[_PeptideAndProteinQuant_PeptideData](new _PeptideAndProteinQuant_PeptideData(deref(it).second))
            
            result[key_str.decode('utf-8')] = py_data
            inc(it)
        
        return result

    def getProteinResults(self):
        """
        getProteinResults(self: PeptideAndProteinQuant) -> dict
        
        Get protein abundance data.
        
        Returns a dictionary mapping protein accessions (as strings) to ProteinData objects.
        """
        cdef libcpp_map[_String, PeptideAndProteinQuant_ProteinData] c_result
        c_result = self.inst.get().getProteinResultsAsStringKeys()
        
        result = {}
        cdef libcpp_map[_String, PeptideAndProteinQuant_ProteinData].iterator it = c_result.begin()
        while it != c_result.end():
            key_str = <bytes>deref(it).first.c_str()
            
            # Create Python wrapper for ProteinData
            py_data = PeptideAndProteinQuant_ProteinData.__new__(PeptideAndProteinQuant_ProteinData)
            py_data.inst = shared_ptr[_PeptideAndProteinQuant_ProteinData](new _PeptideAndProteinQuant_ProteinData(deref(it).second))
            
            result[key_str.decode('utf-8')] = py_data
            inc(it)
        
        return result
