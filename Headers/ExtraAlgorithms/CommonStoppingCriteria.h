// HamiltonFastMarching - A Fast-Marching solver with adaptive stencils.
// Copyright (C) 2017 Jean-Marie Mirebeau, University Paris-Sud, CNRS, University Paris-Saclay.
// Licence GPU GPL v3 or later, see <http://www.gnu.org/licenses/>. Distributed WITHOUT ANY WARRANTY.

#ifndef CommonStoppingCriteria_h
#define CommonStoppingCriteria_h

#include "Base/HFMInterface.h"

template<typename T> struct CommonStoppingCriteria :
HamiltonFastMarching<T>::ExtraAlgorithmInterface {
    typedef HamiltonFastMarching<T> HFM;
    typedef typename HFM::ExtraAlgorithmInterface Superclass;
    Redeclare7Types(FromHFM,IndexCRef,IndexType,ScalarType,Traits,Decision,HFMI,PointType)
    Redeclare1Constant(FromHFM,Dimension)
    
    virtual void Setup(HFMI*) override;
    virtual void Finally(HFMI*) override;
    virtual bool ImplementIn(HFM*) override;
    
    std::set<IndexType> stopWhenAnyAccepted;
    std::set<IndexType> stopWhenAllAccepted;
    ScalarType stopAtDistance = Traits::Infinity(), latestDistance;
    size_t nAccepted=0, nMaxAccepted=std::numeric_limits<size_t>::max();
    std::vector<ScalarType> progressReportLandmarks={0.01,0.02,0.05,0.1,0.2,0.4,0.6,0.8};
protected:
    virtual int PostProcess(IndexCRef) override;
    const HFM * pFM=nullptr;
    std::vector<size_t> nAcceptedLandmarks;
};



template<typename T> bool
CommonStoppingCriteria<T>::ImplementIn(HFM * _pFM) {
    if(stopWhenAllAccepted.empty() &&
       stopWhenAnyAccepted.empty() &&
       stopAtDistance==Traits::Infinity() &&
       nMaxAccepted==std::numeric_limits<size_t>::max() &&
       progressReportLandmarks.empty())
        return false;
    pFM = _pFM;
    for(auto it=progressReportLandmarks.rbegin(); it!=progressReportLandmarks.rend(); ++it)
        nAcceptedLandmarks.push_back(pFM->values.size()* *it);
    return true;
}

template<typename T> int
CommonStoppingCriteria<T>::PostProcess(IndexCRef acceptedIndex){
    assert(pFM!=nullptr);
    ++nAccepted;
    if(nAccepted>=nMaxAccepted)
        return Decision::kTerminate;
    
    if(!nAcceptedLandmarks.empty() && nAccepted==nAcceptedLandmarks.back()){
        Msg() << "Accepted " << std::round(1000.*nAccepted/pFM->values.size())/10.
        << "% of domain points.\n";
        nAcceptedLandmarks.pop_back();
    }
    
    if(stopWhenAnyAccepted.find(acceptedIndex)!=stopWhenAnyAccepted.end())
        return Decision::kTerminate;
    
    if(!stopWhenAllAccepted.empty()){
        stopWhenAllAccepted.erase(acceptedIndex);
        if(stopWhenAllAccepted.empty())
            return Decision::kTerminate;
    }
    
    if(stopAtDistance<Traits::Infinity() && pFM->values(acceptedIndex) > stopAtDistance)
        return Decision::kTerminate;
    
    return Decision::kAccept;
}

template<typename T> void
CommonStoppingCriteria<T>::Setup(HFMI*that){
    auto & io = that->io;
    const auto & dom = that->pFM->dom;
    const auto & param = that->stencil.Param();
    // Stopping criteria
    
    if(io.HasField("nMaxAccepted"))
        nMaxAccepted = io.template Get<ScalarType>("nMaxAccepted");
    
    if(io.HasField("stopWhenAnyAccepted")){
        const auto & targets = io.template GetVector<PointType>("stopWhenAnyAccepted");
        for(const auto & p : targets) {
            IndexType index = dom.IndexFromPoint(param.ADim(p));
            if(dom.Periodize(index)[Dimension]) Msg() << "Warning : target " << p << " is out of range.";
            else stopWhenAnyAccepted.insert(index);
        }
    }
    
    if(io.HasField("stopWhenAllAccepted")){
        const auto & targets = io.template GetVector<PointType>("stopWhenAllAccepted");
        for(const auto & p : targets) {
            IndexType index = dom.IndexFromPoint(param.ADim(p));
            if(dom.Periodize(index)[Dimension]) Msg() << "Warning : target " << p << " is out of range.";
            else stopWhenAllAccepted.insert(index);
        }
    }
    
    if(io.HasField("stopAtDistance"))
        stopAtDistance = io.template Get<ScalarType>("stopAtDistance");
    
    // Other parameters
    
    if(io.HasField("progressReportLandmarks")){
        progressReportLandmarks = io.template GetVector<ScalarType>("progressReportLandmarks");}
    
};

template<typename T> void
CommonStoppingCriteria<T>::Finally(HFMI*that){
    if(pFM!=nullptr) that->io.template Set<ScalarType>("nAccepted",nAccepted);
};



#endif /* CommonStoppingCriteria_h */