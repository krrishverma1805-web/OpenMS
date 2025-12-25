from libcpp.map cimport map as libcpp_map
from cython.operator cimport dereference as deref, preincrement as inc
from AASequence cimport AASequence


    def getPeptideResults(self):
        """
        getPeptideResults(self: PeptideAndProteinQuant) -> dict
        
        Get peptide abundance data.
        
        Returns a dictionary mapping peptide sequences (as strings) to PeptideData objects.
        
        Note: This method manually wraps the C++ getPeptideResults() which returns
        std::map<AASequence, PeptideData>. Since autowrap 0.24 doesn't support maps
        with wrapped classes as both keys and values, we convert AASequence keys to strings.
        """
        # Call C++ method directly - it returns std::map<AASequence, PeptideData>
        cdef libcpp_map[AASequence, PeptideAndProteinQuant_PeptideData] cpp_result
        cpp_result = self.inst.get().getPeptideResults()
        
        # Convert to Python dict with string keys
        py_result = {}
        cdef libcpp_map[AASequence, PeptideAndProteinQuant_PeptideData].iterator it = cpp_result.begin()
        cdef PeptideAndProteinQuant_PeptideData item_result
        cdef AASequence key_seq
        
        while it != cpp_result.end():
            # Get AASequence key and convert to string
            key_seq = deref(it).first
            key_str = key_seq.toString()
            
            # Wrap PeptideData value
            item_result = PeptideAndProteinQuant_PeptideData.__new__(PeptideAndProteinQuant_PeptideData)
            item_result.inst = shared_ptr[_PeptideAndProteinQuant_PeptideData](
                new _PeptideAndProteinQuant_PeptideData(deref(it).second)
            )
            
            py_result[(<bytes>key_str.c_str()).decode('utf-8')] = item_result
            inc(it)
        
        return py_result
