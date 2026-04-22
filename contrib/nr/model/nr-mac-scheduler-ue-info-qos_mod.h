/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nr-mac-scheduler-ue-info-rr.h"

namespace ns3
{
/**
 * \ingroup scheduler
 * \brief UE representation for a QoS-based scheduler
 *
 * The representation stores the current throughput, the average throughput,
 * and the last average throughput, as well as providing comparison functions
 * to sort the UEs in case of a QoS scheduler, according to its QCI and priority.
 *
 * \see CompareUeWeightsDl
 * \see CompareUeWeightsUl
 */
class NrMacSchedulerUeInfoQos : public NrMacSchedulerUeInfo
{
  public:
    /**
     * \brief NrMacSchedulerUeInfoQos constructor
     * \param rnti RNTI of the UE
     * \param beamConfId BeamConfId of the UE
     * \param fn A function that tells how many RB per RBG
     */
    NrMacSchedulerUeInfoQos(float alpha,
                            uint16_t rnti,
                            BeamConfId beamConfId,
                            const GetRbPerRbgFn& fn)
        : NrMacSchedulerUeInfo(rnti, beamConfId, fn),
          m_alpha(alpha)
    {
    }

    /**
     * \brief Reset DL QoS scheduler info
     *
     * Set the last average throughput to the current average throughput,
     * and zeroes the average throughput as well as the current throughput.
     *
     * It calls also NrMacSchedulerUeInfoQos::ResetDlSchedInfo.
     */
    void ResetDlSchedInfo() override
    {
        m_lastAvgTputDl = m_avgTputDl;
        m_avgTputDl = 0.0;
        m_currTputDl = 0.0;
        m_potentialTputDl = 0.0;
        NrMacSchedulerUeInfo::ResetDlSchedInfo();
    }

    /**
     * \brief Reset UL QoS scheduler info
     *
     * Set the last average throughput to the current average throughput,
     * and zeroes the average throughput as well as the current throughput.
     *
     * It also calls NrMacSchedulerUeInfoQos::ResetUlSchedInfo.
     */
    void ResetUlSchedInfo() override
    {
        m_lastAvgTputUl = m_avgTputUl;
        m_avgTputUl = 0.0;
        m_currTputUl = 0.0;
        m_potentialTputUl = 0.0;
        NrMacSchedulerUeInfo::ResetUlSchedInfo();
    }

    /**
     * \brief Reset the DL avg Th to the last value
     */
    void ResetDlMetric() override
    {
        NrMacSchedulerUeInfo::ResetDlMetric();
        m_avgTputDl = m_lastAvgTputDl;
    }

    /**
     * \brief Reset the UL avg Th to the last value
     */
    void ResetUlMetric() override
    {
        NrMacSchedulerUeInfo::ResetUlMetric();
        m_avgTputUl = m_lastAvgTputUl;
    }

    /**
     * \brief Update the QoS metric for downlink
     * \param totAssigned the resources assigned
     * \param timeWindow the time window
     * \param amc a pointer to the AMC
     *
     * Updates m_currTputDl and m_avgTputDl by keeping in consideration
     * the assigned resources (in form of TBS) and the time window.
     * It gets the tbSise by calling NrMacSchedulerUeInfo::UpdateDlMetric.
     */
    void UpdateDlQosMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                           double timeWindow,
                           const Ptr<const NrAmc>& amc);

    /**
     * \brief Update the QoS metric for uplink
     * \param totAssigned the resources assigned
     * \param timeWindow the time window
     * \param amc a pointer to the AMC
     *
     * Updates m_currTputUl and m_avgTputUl by keeping in consideration
     * the assigned resources (in form of TBS) and the time window.
     * It gets the tbSise by calling NrMacSchedulerUeInfo::UpdateUlMetric.
     */
    void UpdateUlQosMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                           double timeWindow,
                           const Ptr<const NrAmc>& amc);

    /**
     * \brief Calculate the Potential throughput for downlink
     * \param assignableInIteration resources assignable
     * \param amc a pointer to the AMC
     */
    void CalculatePotentialTPutDl(const NrMacSchedulerNs3::FTResources& assignableInIteration,
                                  const Ptr<const NrAmc>& amc);

    /**
     * \brief Calculate the Potential throughput for uplink
     * \param assignableInIteration resources assignable
     * \param amc a pointer to the AMC
     */
    void CalculatePotentialTPutUl(const NrMacSchedulerNs3::FTResources& assignableInIteration,
                                  const Ptr<const NrAmc>& amc);

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the QoS metric of the left UE is higher than the right UE
     *
     * The QoS metric is calculated in CalculateDlWeight()
     */
    static bool CompareUeWeightsDl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        // std::cout << "CalculateDlWeight(" << lue.first->m_rnti << ", " << rue.first->m_rnti << ")" << std::endl;
        // std::cout << "lue: CalculateDlWeight(UE" << lue.first->m_rnti << ")" << std::endl;
        double lQoSMetric = CalculateDlWeight(lue);
        // std::cout << "rue: CalculateDlWeight(UE" << rue.first->m_rnti << ")" << std::endl;
        double rQoSMetric = CalculateDlWeight(rue);

        // std::cout << "UE " << lue.first->m_rnti << " tiene W = " << lQoSMetric << std::endl;
        // std::cout << "UE " << rue.first->m_rnti << " tiene W = " << rQoSMetric << std::endl;

        NS_ASSERT_MSG(lQoSMetric > 0, "Weight must be greater than zero");
        NS_ASSERT_MSG(rQoSMetric > 0, "Weight must be greater than zero");

        // std::cout << "Devuelve " << (lQoSMetric > rQoSMetric) << std::endl;
        return (lQoSMetric > rQoSMetric);
    }
    void SetSymbolsPerSec(double symbols) {
        m_symbolsPerSec = symbols;
    }

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \f$ qosMetric_{i} = P * std::pow(potentialTPut_{i}, alpha) / std::max (1E-9, m_avgTput_{i})
     * \f$
     *
     * Alpha is a fairness metric. P is the priority associated to the QCI.
     * Please note that the throughput is calculated in bit/symbol.
    //  */
    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param ue UE
     * \f$ qosMetric_{i} = P * std::pow(potentialTPut_{i}, alpha) / std::max (1E-9, m_avgTput_{i})
     * \f$
     *
     * Alpha is a fairness metric. P is the priority associated to the QCI.
     * Please note that the throughput is calculated in bit/symbol.
     */
    // static double CalculateDlWeight(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    // {
    //     double weight = 0;
    //     auto uePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(ue.first.get());

    //     // OUTER LOOP: Iterate over all Logical Channel Groups (LCGs) for this UE
    //     for (const auto& ueLcg : ue.first->m_dlLCG)
    //     {
    //         // Here is where ueActiveLCs is defined!
    //         std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

    //         // INNER LOOP: Iterate over the active Logical Channels
    //         for (const auto lcId : ueActiveLCs)
    //         {
    //             std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);
    //             double delayBudgetFactor = 1.0;
                
    //             if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR)
    //             {
    //                 delayBudgetFactor =
    //                     CalculateDelayBudgetFactor(LCPtr->m_delayBudget.GetMilliSeconds(),
    //                                                LCPtr->m_rlcTransmissionQueueHolDelay);
    //             }

    //             // 1. Calculate standard QoS Weight
    //             double baseWeight = (100 - LCPtr->m_priority) *
    //                                 std::pow(uePtr->m_potentialTputDl, uePtr->m_alpha) /
    //                                 std::max(1E-9, uePtr->m_avgTputDl) * delayBudgetFactor;

    //             // 2. Apply Dynamic GFBR Boost if it is a GBR bearer
    //             if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_GBR ||
    //                 LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR)
    //             {
    //                 double currentGfbr = 0.0;
    //                 if (uePtr->m_dynamicGfbrDl.count(lcId) > 0) {
    //                     currentGfbr = uePtr->m_dynamicGfbrDl[lcId];
    //                 }

    //                 // If average throughput is below the dynamic GFBR, boost the weight
    //                 if (currentGfbr > 0 && uePtr->m_avgTputDl < currentGfbr) {
    //                     double boostFactor = currentGfbr / std::max(1E-9, uePtr->m_avgTputDl);
    //                     // Cap the boost factor to prevent extreme starvation of other UEs
    //                     boostFactor = std::min(boostFactor, 100.0); 
    //                     baseWeight *= boostFactor;
    //                 }
    //             }

    //             weight += baseWeight;
    //             NS_ASSERT_MSG(weight > 0, "Weight must be greater than zero");
    //         }
    //     }
    //     return weight;
    // }
//     static double CalculateDlWeight(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
// {
//     double weight = 0;
//     auto uePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(ue.first.get());

//     for (const auto& ueLcg : ue.first->m_dlLCG)
//     {
//         std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();
//         // for (const auto lcId : ueActiveLCs)
//         // {
//         //     std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);
//         //     double delayBudgetFactor = 1.0;

//         //     if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR) {
//         //         delayBudgetFactor = CalculateDelayBudgetFactor(LCPtr->m_delayBudget.GetMilliSeconds(),
//         //                                                        LCPtr->m_rlcTransmissionQueueHolDelay);
//         //     }

//         //     // Standard QoS Weight
//         //     double baseWeight = (100 - LCPtr->m_priority) *
//         //                         std::pow(uePtr->m_potentialTputDl, uePtr->m_alpha) /
//         //                         std::max(1E-9, uePtr->m_avgTputDl) * delayBudgetFactor;

//         //     // Dynamic GFBR Boost
//         //     if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_GBR ||
//         //         LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR)
//         //     {
//         //         double currentGfbr = uePtr->m_dynamicGfbrDl.count(lcId) ? uePtr->m_dynamicGfbrDl[lcId] : 0.0;
//         //         if (currentGfbr > 0 && uePtr->m_avgTputDl < currentGfbr) {
//         //             double boostFactor = std::min(currentGfbr / std::max(1E-9, uePtr->m_avgTputDl), 100.0); 
//         //             baseWeight *= boostFactor;
//         //         }
//         //     }
//         //     weight += baseWeight;
//         // }
        
//         // 1. Set your numerology symbols per second (28000 is for 30kHz numerology)
//         // If you are using 15kHz numerology, change this to 14000.0
//         const double SYMBOLS_PER_SEC = 28000.0; 

//         for (const auto lcId : ueActiveLCs)
//         {
//             std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);
//             double delayBudgetFactor = 1.0;
            
//             if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR) {
//                 delayBudgetFactor = CalculateDelayBudgetFactor(LCPtr->m_delayBudget.GetMilliSeconds(),
//                                                             LCPtr->m_rlcTransmissionQueueHolDelay);
//             }

//             // 2. Calculate standard QoS Weight for normal (non-starving) traffic
//             double baseWeight = (100 - LCPtr->m_priority) *
//                                 std::pow(uePtr->m_potentialTputDl, uePtr->m_alpha) /
//                                 std::max(1E-9, uePtr->m_avgTputDl) * delayBudgetFactor;

//             // // 3. Apply STRICT PRIORITY for GFBR
//             // if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_GBR ||
//             //     LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR)
//             // {
//             //     double currentGfbrBps = 0.0;
//             //     if (uePtr->m_dynamicGfbrDl.count(lcId) > 0) {
//             //         currentGfbrBps = uePtr->m_dynamicGfbrDl[lcId];
//             //     }

//             //     // Convert the scheduler's bits/symbol into bits/sec to compare correctly
//             //     double currentAvgTputBps = uePtr->m_avgTputDl * SYMBOLS_PER_SEC;

//             //     // If the UE is starving for its requested GFBR
//             //     if (currentGfbrBps > 0 && currentAvgTputBps < currentGfbrBps) {
                    
//             //         // Calculate exactly how many bits per second it is missing
//             //         double deficitBps = currentGfbrBps - currentAvgTputBps;
                    
//             //         // OVERRIDE the weight. 
//             //         // The massive 1,000,000 base ensures starving UEs always beat non-starving ones.
//             //         // Adding the deficit ensures that if UE1 and UE2 are BOTH starving, 
//             //         // the one missing the most throughput gets the resources first.
//             //         baseWeight = 1000000.0 + deficitBps; 
//             //     }
//             // }
//             // 3. Apply STRICT PRIORITY for GFBR
//             if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_GBR ||
//                 LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR)
//             {
//                 double currentGfbrBps = 0.0;
//                 if (uePtr->m_dynamicGfbrDl.count(lcId) > 0) {
//                     currentGfbrBps = uePtr->m_dynamicGfbrDl[lcId];
//                 }

//                 // Convert the scheduler's bits/symbol into bits/sec to compare correctly
//                 double currentAvgTputBps = uePtr->m_avgTputDl * SYMBOLS_PER_SEC;

//                 // // If the UE is starving for its requested GFBR
//                 // if (currentGfbrBps > 0 && currentAvgTputBps < currentGfbrBps) {
                    
//                 //     // Calculate exactly how many bits per second it is missing
//                 //     double deficitBps = currentGfbrBps - currentAvgTputBps;
                    
//                 //     // OVERRIDE the weight. 
//                 //     // We use 1e15 (1 Quadrillion) to absolutely guarantee that a starving 
//                 //     // GFBR UE will always mathematically crush any non-GFBR UE, 
//                 //     // even if the non-GFBR UE's Proportional Fair denominator hits 1E-9.
//                 //     baseWeight = 1e15 + deficitBps; 
//                 // }
//                 // If the UE is starving for its requested GFBR
//                 if (currentGfbrBps > 0 && currentAvgTputBps < currentGfbrBps) {
                    
//                     // Calculate what percentage of its GFBR target is missing (Range: 0.0 to 1.0)
//                     // Example: Wants 4M, has 0M -> 1.0 (100% starved)
//                     // Example: Wants 30M, has 15M -> 0.5 (50% starved)
//                     double deficitBps = currentGfbrBps - currentAvgTputBps;
//                     double starvationPercentage = deficitBps / currentGfbrBps;
                    
//                     // OVERRIDE the weight. 
//                     // 1e15 guarantees we always beat NGBR UEs.
//                     // Multiplying starvationPercentage by 1e10 ensures that the GBR UE 
//                     // that is relatively the most starved gets the resources first.
//                     baseWeight = 1e15 + (starvationPercentage * 1e10); 
//                 }
//             }

//             weight += baseWeight;
//             NS_ASSERT_MSG(weight > 0, "Weight must be greater than zero");
//         }
//     }
//     return weight;
// }
static double CalculateDlWeight(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        double weight = 0.0;
        auto uePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(ue.first.get());
        
        // 1. Set your numerology symbols per second (28000 is for 30kHz)
        // const double SYMBOLS_PER_SEC = 28000.0; 
        const double SYMBOLS_PER_SEC = 14000.0;  // If you are using 15kHz numerology, change this to 14000.0
        

        for (const auto& ueLcg : ue.first->m_dlLCG)
        {
            std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();
            for (const auto lcId : ueActiveLCs)
            {
                std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);
                double delayBudgetFactor = 1.0;
                
                if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR) {
                    delayBudgetFactor = CalculateDelayBudgetFactor(LCPtr->m_delayBudget.GetMilliSeconds(),
                                                                   LCPtr->m_rlcTransmissionQueueHolDelay);
                }

                // 2. Calculate standard QoS Weight
                double baseWeight = (100.0 - LCPtr->m_priority) *
                                    std::pow(uePtr->m_potentialTputDl, uePtr->m_alpha) /
                                    std::max(1E-9, uePtr->m_avgTputDl) * delayBudgetFactor;

                // --- SAFETY NET ---
                // If standard math evaluates to 0 or NaN, give it a tiny valid weight
                if (std::isnan(baseWeight) || baseWeight <= 0.0) {
                    baseWeight = 1e-9;
                }

                // 3. Apply STRICT PRIORITY for GFBR (Proportional Starvation)
                if (LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_GBR ||
                    LCPtr->m_resourceType == LogicalChannelConfigListElement_s::QBT_DGBR)
                {
                    double currentGfbrBps = 0.0;
                    if (uePtr->m_dynamicGfbrDl.count(lcId) > 0) {
                        currentGfbrBps = uePtr->m_dynamicGfbrDl[lcId];
                    }

                    double currentAvgTputBps = uePtr->m_avgTputDl * SYMBOLS_PER_SEC;

                    // // If the UE is starving for its requested GFBR
                    // if (currentGfbrBps > 0 && currentAvgTputBps < currentGfbrBps) {
                        
                    //     double deficitBps = currentGfbrBps - currentAvgTputBps;
                    //     double starvationPercentage = deficitBps / currentGfbrBps;
                        
                    //     // OVERRIDE the weight based on starvation percentage
                    //     baseWeight = 1e15 + (starvationPercentage * 1e10); 
                    // }
                    // If the UE is starving for its requested GFBR
                    if (currentGfbrBps > 0 && currentAvgTputBps < currentGfbrBps) {
                        
                        double deficitBps = currentGfbrBps - currentAvgTputBps;
                        double starvationPercentage = deficitBps / currentGfbrBps;
                        
                        // Calculate a tie-breaker based on channel quality and past throughput
                        double pfTieBreaker = std::pow(uePtr->m_potentialTputDl, uePtr->m_alpha) / 
                                              std::max(1E-9, uePtr->m_avgTputDl);
                        
                        // OVERRIDE the weight.
                        // 1e15: Guarantees GFBR UEs beat non-GBR UEs.
                        // 1e10: Prioritizes the most starved GFBR UEs.
                        // pfTieBreaker: Safely breaks ties if multiple UEs are equally starved!
                        baseWeight = 1e15 + (starvationPercentage * 1e10) + pfTieBreaker; 
                    }
                }

                weight += baseWeight;
            }
        }
        
        // Final safety net: absolutely guarantee the final returned weight is > 0
        return std::max(1e-9, weight);
    }

    /**
     * \brief This function calculates the Delay Budget Factor for the case of
     * DC-GBR LC. This value will then be used for the calculation of the QoS
     * metric (weight).
     * Notice that in order to avoid the case that a packet has not been dropped
     * when HOL >= PDB, even though it is in this state (currently our code does
     * not implement packet drop by default), we give very high priority to this
     * packet. We do this by considering a very small value for the denominator
     * (i.e. (PDB - HOL) = 0.1).
     * \param pdb The Packet Delay Budget associated to the QCI
     * \param hol The HeadOfLine Delay of the transmission queue
     * \return the delayBudgetFactor
     */
    static double CalculateDelayBudgetFactor(uint64_t pdb, uint16_t hol)
    {
        double denominator = hol >= pdb ? 0.1 : static_cast<double>(pdb) - static_cast<double>(hol);
        double delayBudgetFactor = static_cast<double>(pdb) / denominator;

        return delayBudgetFactor;
    }

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the QoS metric of the left UE is higher than the right UE
     *
     * The QoS metric is calculated as following:
     *
     * \f$ qosMetric_{i} = P * std::pow(potentialTPut_{i}, alpha) / std::max (1E-9, m_avgTput_{i})
     * \f$
     *
     * Alpha is a fairness metric. P is the priority associated to the QCI.
     * Please note that the throughput is calculated in bit/symbol.
     */
    static bool CompareUeWeightsUl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        auto luePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(lue.first.get());
        auto ruePtr = dynamic_cast<NrMacSchedulerUeInfoQos*>(rue.first.get());

        double leftP = CalculateUlMinPriority(lue);
        double rightP = CalculateUlMinPriority(rue);
        NS_ABORT_IF(leftP == 0);
        NS_ABORT_IF(rightP == 0);

        // double lQoSMetric = (100 - leftP) * std::pow(luePtr->m_potentialTputUl, luePtr->m_alpha) /
        //                     std::max(1E-9, luePtr->m_avgTputUl);


        double lQoSMetric = (100 - leftP) * std::pow(luePtr->m_potentialTputUl, luePtr->m_alpha) /
                    std::max(1E-9, luePtr->m_avgTputUl);

        // Check if Left UE needs an UL GFBR boost (checking the first GBR LC for simplicity)
        for (const auto& pair : luePtr->m_dynamicGfbrUl) {
            if (luePtr->m_avgTputUl < pair.second) {
                lQoSMetric *= std::min(pair.second / std::max(1E-9, luePtr->m_avgTputUl), 100.0);
                break; // Apply boost based on the highest priority starving LC
            }
        }
        double rQoSMetric = (100 - rightP) * std::pow(ruePtr->m_potentialTputUl, ruePtr->m_alpha) /
                            std::max(1E-9, ruePtr->m_avgTputUl);
        // Check if Right UE needs an UL GFBR boost (checking the first GBR LC for simplicity)
        for (const auto& pair : ruePtr->m_dynamicGfbrUl) {
            if (ruePtr->m_avgTputUl < pair.second) {
                rQoSMetric *= std::min(pair.second / std::max(1E-9, ruePtr->m_avgTputUl), 100.0);
                break; // Apply boost based on the highest priority starving LC
            }
        }

        return (lQoSMetric > rQoSMetric);
    }

    /**
     * \brief This function calculates the min Priority for the DL.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the Priority of lue is less than the Priority of rue
     *
     * The ordering is made by considering the minimum Priority among all the
     * Priorities of all the LCs set for this UE.
     * A UE that has a Priority = 5 will always be the first (i.e., has a higher
     * priority) in a QoS scheduler.
     */
    static uint8_t CalculateDlMinPriority(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        uint8_t ueMinPriority = 100;

        for (const auto& ueLcg : ue.first->m_dlLCG)
        {
            std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

            for (const auto lcId : ueActiveLCs)
            {
                std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);

                if (ueMinPriority > LCPtr->m_priority)
                {
                    ueMinPriority = LCPtr->m_priority;
                }

                ue.first->PrintLcInfo(ue.first->m_rnti,
                                      ueLcg.first,
                                      lcId,
                                      LCPtr->m_qci,
                                      LCPtr->m_priority,
                                      ueMinPriority);
            }
        }
        return ueMinPriority;
    }

    /**
     * \brief This function calculates the min Priority for the UL.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the Priority of lue is less than the Priority of rue
     *
     * The ordering is made by considering the minimum Priority among all the
     * Priorities of all the LCs set for this UE.
     * A UE that has a Priority = 5 will always be the first (i.e., has a higher
     * priority) in a QoS scheduler.
     */
    static uint8_t CalculateUlMinPriority(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        uint8_t ueMinPriority = 100;

        for (const auto& ueLcg : ue.first->m_ulLCG)
        {
            std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

            for (const auto lcId : ueActiveLCs)
            {
                std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);

                if (ueMinPriority > LCPtr->m_priority)
                {
                    ueMinPriority = LCPtr->m_priority;
                }

                ue.first->PrintLcInfo(ue.first->m_rnti,
                                      ueLcg.first,
                                      lcId,
                                      LCPtr->m_qci,
                                      LCPtr->m_priority,
                                      ueMinPriority);
            }
        }
        return ueMinPriority;
    }

    double m_currTputDl{0.0};      //!< Current slot throughput in downlink
    double m_avgTputDl{0.0};       //!< Average throughput in downlink during all the slots
    double m_lastAvgTputDl{0.0};   //!< Last average throughput in downlink
    double m_potentialTputDl{0.0}; //!< Potential throughput in downlink in one assignable resource
                                   //!< (can be a symbol or a RBG)
    float m_alpha{0.0};            //!< PF fairness metric

    double m_currTputUl{0.0};      //!< Current slot throughput in uplink
    double m_avgTputUl{0.0};       //!< Average throughput in uplink during all the slots
    double m_lastAvgTputUl{0.0};   //!< Last average throughput in uplink
    double m_potentialTputUl{0.0}; //!< Potential throughput in uplink in one assignable resource
                                   //!< (can be a symbol or a RBG)

    /**
     * \brief Update the dynamic GFBR for a specific Downlink LC
     */
    void UpdateUeDlGfbr(uint8_t lcId, double gfbrBps) {
        m_dynamicGfbrDl[lcId] = gfbrBps;
    }

    /**
     * \brief Update the dynamic GFBR for a specific Uplink LC
     */
    void UpdateUeUlGfbr(uint8_t lcId, double gfbrBps) {
        m_dynamicGfbrUl[lcId] = gfbrBps;
    }

    std::map<uint8_t, double> m_dynamicGfbrDl; //!< Dynamic GFBR per LCID for DL
    std::map<uint8_t, double> m_dynamicGfbrUl; //!< Dynamic GFBR per LCID for UL
    double m_symbolsPerSec {14000}; // Default fallback
};

} // namespace ns3
